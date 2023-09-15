use std::collections::HashMap;
use std::env::current_dir;
use std::fs::File;
use std::io::Read;
use std::sync::Mutex;
use std::thread;
use std::time::Duration;

use cxx::let_cxx_string;

use config::{Config, ConfigError};
use futures::StreamExt;
use lazy_static::lazy_static;
use log::debug;
use log::warn;
use tokio::spawn;
use tokio::sync::mpsc;
use tokio::sync::mpsc::Sender;
use tokio::time::sleep;
use tonic::Request;

use crate::api::bambu_farm::{ConnectRequest, SendMessageRequest, UploadFileRequest};
use crate::api::ffi::bambu_network_cb_connected;
use crate::api::ffi::bambu_network_cb_message_recv;
use crate::BAMBU_NETWORK_ERR_SEND_MSG_FAILED;
use crate::BAMBU_NETWORK_SUCCESS;

use self::bambu_farm::bambu_farm_client::BambuFarmClient;
use self::bambu_farm::PrinterOptionRequest;
use self::ffi::bambu_network_cb_printer_available;

pub mod bambu_farm {
    tonic::include_proto!("_");
}

lazy_static! {
    static ref RUNTIME: tokio::runtime::Runtime = tokio::runtime::Builder::new_multi_thread()
        .worker_threads(6)
        .enable_all()
        .build()
        .unwrap();
}

lazy_static! {
    static ref MSG_TX: Mutex<HashMap<String, Sender<SendMessageRequest>>> =
        Mutex::new(HashMap::new());
}

#[cxx::bridge]
mod ffi {

    extern "Rust" {
        pub fn bambu_network_rs_init();
        pub fn bambu_network_rs_log_debug(message: String);
        pub fn bambu_network_rs_connect(device_id: String) -> i32;
        pub fn bambu_network_rs_send(device_id: String, data: String) -> i32;
        pub fn bambu_network_rs_upload_file(
            device_id: String,
            local_filename: String,
            remote_filename: String,
        ) -> i32;
    }

    unsafe extern "C++" {
        include!("api.hpp");

        pub fn bambu_network_cb_printer_available(json: &CxxString);
        pub fn bambu_network_cb_message_recv(device_id: &CxxString, json: &CxxString);
        pub fn bambu_network_cb_connected(device_id: &CxxString);
    }
}

pub fn get_endpoint() -> String {
    let config_file_path = if let Ok(dir) = current_dir() {
        dir.join("bambufarm.toml")
    } else {
        "bambufarm.toml".into()
    };

    let config = match Config::builder()
        .add_source(config::File::with_name(&config_file_path.to_string_lossy()).required(false))
        .add_source(config::Environment::with_prefix("BAMBU_FARM"))
        .build()
    {
        Ok(config) => config,
        Err(err) => {
            match err {
                ConfigError::NotFound(_) => {
                    warn!(
                        "No config file found. Try adding one at `{}`",
                        config_file_path.to_string_lossy()
                    );
                }
                ConfigError::FileParse { uri, cause } => {
                    if let Some(uri) = uri {
                        warn!("Error parsing config file at {}\nCause:{}", uri, cause);
                    } else {
                        warn!("Error parsing config file.")
                    }
                }
                _ => {
                    warn!("Unknown error parsing config file {:?}", err)
                }
            }
            return "http://[::1]:47403".into();
        }
    };
    config
        .get_string("endpoint")
        .map(|socket_addr| {
            if config.get_bool("use_https").unwrap_or(false) {
                format!("https://{}", socket_addr)
            } else {
                format!("http://{}", socket_addr)
            }
        })
        .unwrap_or("http://[::1]:47403".into())
}

pub fn bambu_network_rs_init() {
    debug!("Calling network init");
    RUNTIME.spawn(async {
        debug!("Connecting to localhost.");
        thread::sleep(Duration::from_secs(1));
        debug!("Connecting to localhost.");
        let mut client = match BambuFarmClient::connect(get_endpoint()).await {
            Ok(client) => client,
            Err(_) => {
                warn!("Failed to connect to farm.");
                return;
            }
        };

        debug!("Requesting available printers.");
        let request = Request::new(PrinterOptionRequest {});
        let mut stream = match client.get_available_printers(request).await {
            Ok(stream) => stream.into_inner(),
            Err(_) => {
                debug!("Error while fetching available printers.");
                return;
            }
        };
        loop {
            let list = stream.next().await;
            let list = match list {
                Some(Ok(list)) => list,
                _ => {
                    sleep(Duration::from_secs(1)).await;
                    continue;
                }
            };
            for printer in list.options {
                let_cxx_string!(
                    json = format!(
                        "{}
                        \"dev_name\": \"{}\",
                        \"dev_id\": \"{}\",
                        \"dev_ip\": \"127.0.0.1\",
                        \"dev_type\": \"3DPrinter-X1-Carbon\",
                        \"dev_signal\": \"0dbm\",
                        \"connect_type\": \"lan\",
                        \"bind_state\": \"free\"
                        {}",
                        "{", printer.dev_name, printer.dev_id, "}"
                    )
                    .trim()
                    .as_bytes()
                );
                bambu_network_cb_printer_available(&json);
            }
        }
    });
}

pub fn bambu_network_rs_log_debug(message: String) {
    debug!("cxx: {}", message);
}

pub fn bambu_network_rs_connect(device_id: String) -> i32 {
    debug!("Attempting connection.");

    let (tx, mut rx) = mpsc::channel(10);
    MSG_TX.lock().unwrap().insert(device_id.clone(), tx);

    RUNTIME.spawn(async move {
        let mut client = match BambuFarmClient::connect("http://[::1]:47403").await {
            Ok(client) => client,
            Err(_) => {
                warn!("Failed to connect to farm.");
                return;
            }
        };

        let request = Request::new(ConnectRequest {
            dev_id: device_id.clone(),
        });
        let mut stream = match client.connect_printer(request).await {
            Ok(stream) => stream.into_inner(),
            Err(_) => {
                warn!("Error while fetching recieved messages.");
                return;
            }
        };
        {
            let_cxx_string!(device_id_cxx = device_id.clone());
            bambu_network_cb_connected(&device_id_cxx);
        }
        spawn(async move {
            loop {
                if let Some(message) = rx.recv().await {
                    debug!("Sending message: {}", message.data);
                    let response = match client.send_message(message).await {
                        Ok(response) => response.into_inner(),
                        Err(_) => {
                            warn!("Error while sending message.");
                            return;
                        }
                    };
                }
            }
        });
        loop {
            let recv_message = stream.next().await;
            let recv_message = match recv_message {
                Some(Ok(recv_message)) => recv_message,
                _ => {
                    sleep(Duration::from_secs(10)).await;
                    continue;
                }
            };
            if !recv_message.connected {
                break;
            }
            let_cxx_string!(id = recv_message.dev_id);
            let_cxx_string!(json = recv_message.data);
            bambu_network_cb_message_recv(&id, &json);
        }
    });
    0
}

pub fn bambu_network_rs_send(device_id: String, data: String) -> i32 {
    debug!("Sending {}", data);

    RUNTIME.block_on(async {
        let sender = if let Some(sender) = MSG_TX.lock().unwrap().get(&device_id) {
            sender.clone()
        } else {
            return BAMBU_NETWORK_ERR_SEND_MSG_FAILED;
        };
        let result = sender
            .send(SendMessageRequest {
                dev_id: device_id,
                data,
            })
            .await;
        if result.is_ok() {
            return BAMBU_NETWORK_SUCCESS;
        } else {
            return BAMBU_NETWORK_ERR_SEND_MSG_FAILED;
        }
    })
}

pub fn bambu_network_rs_upload_file(
    device_id: String,
    local_filename: String,
    remote_filename: String,
) -> i32 {
    RUNTIME.block_on(async {
        let mut client = match BambuFarmClient::connect("http://[::1]:47403").await {
            Ok(client) => client,
            Err(_) => {
                warn!("Failed to connect to farm.");
                return BAMBU_NETWORK_ERR_SEND_MSG_FAILED;
            }
        };

        let mut blob = vec![];
        File::open(local_filename)
            .unwrap()
            .read_to_end(&mut blob)
            .unwrap();

        let result = client
            .upload_file(UploadFileRequest {
                dev_id: device_id,
                blob,
                remote_path: dbg!(remote_filename),
            })
            .await;
        if result.is_ok() {
            return BAMBU_NETWORK_SUCCESS;
        } else {
            return BAMBU_NETWORK_ERR_SEND_MSG_FAILED;
        }
    })
}
