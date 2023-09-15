use bambu_farm::PrinterOption;
use bambu_farm::{
    bambu_farm_server::BambuFarm, ConnectRequest, PrinterOptionList, PrinterOptionRequest,
    RecvMessage, SendMessageRequest, SendMessageResponse, UploadFileRequest, UploadFileResponse,
};
use config::{Config, ConfigError, Value};
use paho_mqtt::{
    AsyncClient, ConnectOptionsBuilder, CreateOptionsBuilder, Message, SslOptionsBuilder,
};
use tempfile::NamedTempFile;
use tokio::spawn;
use tokio::time::sleep;

use crate::bambu_farm::bambu_farm_server::BambuFarmServer;
use std::collections::HashMap;
use std::env::current_dir;
use std::io::Write;
use std::process::Command;
use std::sync::Arc;
use std::sync::Mutex;
use std::thread;
use std::time::Duration;
use tokio::sync::mpsc;
use tokio_stream::wrappers::ReceiverStream;
use tonic::{transport::Server, Request, Response, Status};

pub mod bambu_farm {
    tonic::include_proto!("_");
}

#[derive(Debug, Clone)]
struct Printer {
    name: String,
    id: String,
    ip: String,
    model: String,
    password: String,
}

#[derive(Default)]
pub struct Farm {
    printers: Arc<Mutex<Vec<Printer>>>,
    connections: Arc<Mutex<HashMap<String, tokio::sync::mpsc::Sender<SendMessageRequest>>>>,
}

#[tonic::async_trait]
impl BambuFarm for Farm {
    type GetAvailablePrintersStream = ReceiverStream<Result<PrinterOptionList, Status>>;

    async fn get_available_printers(
        &self,
        _request: Request<PrinterOptionRequest>,
    ) -> Result<Response<Self::GetAvailablePrintersStream>, Status> {
        let (tx, rx) = mpsc::channel(4);

        let printers = self.printers.clone();
        tokio::spawn(async move {
            loop {
                let printers = match printers.lock() {
                    Ok(printers) => printers.clone(),
                    Err(_) => return,
                };
                let list = PrinterOptionList {
                    options: printers
                        .iter()
                        .map(|printer| PrinterOption {
                            dev_name: printer.name.clone(),
                            dev_id: printer.id.clone(),
                            model: printer.model.clone(),
                        })
                        .collect(),
                };
                if tx.send(Ok(list)).await.is_err() {
                    return;
                }
                sleep(Duration::from_secs(1)).await;
            }
        });

        Ok(Response::new(ReceiverStream::new(rx)))
    }

    type ConnectPrinterStream = ReceiverStream<Result<RecvMessage, Status>>;

    async fn connect_printer(
        &self,
        request: Request<ConnectRequest>,
    ) -> Result<Response<Self::ConnectPrinterStream>, Status> {
        let (response_tx, response_rx) = mpsc::channel(4);

        let printers = match self.printers.lock() {
            Ok(printers) => printers.clone(),
            Err(_) => return Err(Status::unknown("Could not lock printer list.")),
        };
        let printer = printers
            .iter()
            .filter(|printer| printer.id == request.get_ref().dev_id)
            .next();
        if let Some(printer) = printer {
            let dev_id = printer.id.clone();
            let options =
                CreateOptionsBuilder::new().server_uri(format!("mqtts://{}:8883", printer.ip));
            let mut client = AsyncClient::new(options.finalize()).unwrap();

            let (message_tx, mut message_rx) = tokio::sync::mpsc::channel(1);

            let connection_token = client.connect(Some(
                ConnectOptionsBuilder::new()
                    .user_name("bblp")
                    .password(&printer.password)
                    .ssl_options(
                        SslOptionsBuilder::new()
                            .verify(false)
                            .enable_server_cert_auth(false)
                            .finalize(),
                    )
                    .finalize(),
            ));
            connection_token.wait().unwrap();

            client
                .subscribe(format!("device/{}/report", request.get_ref().dev_id), 0)
                .await
                .unwrap();

            let stream = client.get_stream(100);

            spawn(async move {
                loop {
                    if let Ok(Some(message)) = stream.recv().await {
                        response_tx
                            .send(Ok(RecvMessage {
                                connected: true,
                                dev_id: dev_id.clone(),
                                data: message.payload_str().to_string(),
                            }))
                            .await
                            .unwrap();
                    } else {
                        println!("Connection broken");
                        break;
                    };
                }
                println!("Connection broken.");
            });

            self.connections
                .lock()
                .unwrap()
                .insert(request.get_ref().dev_id.clone(), message_tx);

            spawn(async move {
                loop {
                    let recv_message = message_rx.recv().await;
                    let recv_message = match recv_message {
                        Some(recv_message) => recv_message,
                        _ => {
                            sleep(Duration::from_secs(10)).await;
                            continue;
                        }
                    };
                    println!("Publishing to topic");
                    client
                        .publish(Message::new(
                            format!("device/{}/request", request.get_ref().dev_id),
                            recv_message.data,
                            0,
                        ))
                        .await
                        .unwrap();
                }
            });
        } else {
            return Err(Status::unknown("No matching printer."));
        }

        Ok(Response::new(ReceiverStream::new(response_rx)))
    }

    async fn send_message(
        &self,
        request: Request<SendMessageRequest>,
    ) -> Result<Response<SendMessageResponse>, Status> {
        let client = {
            let request = request.get_ref();
            let connections = self.connections.lock().unwrap();
            connections.get(&request.dev_id).unwrap().clone()
        };
        let request = request.get_ref();
        match client.send(request.clone()).await {
            Ok(_) => Ok(Response::new(SendMessageResponse { success: true })),
            Err(_) => Ok(Response::new(SendMessageResponse { success: false })),
        }
    }

    async fn upload_file(
        &self,
        request: Request<UploadFileRequest>,
    ) -> Result<Response<UploadFileResponse>, Status> {
        let printers = match self.printers.lock() {
            Ok(printers) => printers.clone(),
            Err(_) => return Err(Status::unknown("Could not lock printer list.")),
        };
        let printer = printers
            .iter()
            .filter(|printer| printer.id == request.get_ref().dev_id)
            .next();
        if let Some(printer) = printer {
            let printer = printer.clone();
            let handle = thread::spawn(move || {
                let _ = Command::new("curl")
                    .args([
                        "--insecure",
                        &format!("ftps://bblp:{}@{}/", printer.password, printer.ip),
                        "-Q",
                        &format!("\"DELE {}\"", request.get_ref().remote_path),
                    ])
                    .output()
                    .unwrap();
                let mut temp = NamedTempFile::new().unwrap();
                temp.write_all(&request.get_ref().blob).unwrap();
                temp.flush().unwrap();
                let output = Command::new("curl")
                    .args(dbg!([
                        "--insecure",
                        &format!(
                            "ftps://bblp:{}@{}/{}",
                            printer.password,
                            printer.ip,
                            request.get_ref().remote_path
                        ),
                        "-T",
                        temp.path().to_str().unwrap(),
                    ]))
                    .output()
                    .unwrap();
                Ok(Response::new(UploadFileResponse {
                    success: output.status.success(),
                }))
            });
            while !handle.is_finished() {
                println!("Sleeping waiting for upload to finish");
                sleep(Duration::from_secs(1)).await;
            }
            println!("Done waiting for upload");
            handle.join().unwrap()
        } else {
            println!("No matching printer.");
            Err(Status::unknown("Could not find matching printer"))
        }
    }
}

fn construct_printer(config: Value) -> Option<Printer> {
    let printer = config.into_table().unwrap_or_default();

    let dev_id = if let Some(dev_id) = printer.get("dev_id") {
        dev_id
    } else {
        eprintln!("Missing `dev_id` in printer config.");
        return None;
    };
    let model = if let Some(model) = printer.get("model") {
        match model.to_string().to_lowercase().as_str() {
            "x1c" => "3DPrinter-X1-Carbon",
            "x1" => "3DPrinter-X1",
            _ => {
                eprintln!("Expected printer field `model` to be one of [`x1`, `x1c`].");
                return None;
            }
        }
    } else {
        eprintln!("Missing `model` in printer config.");
        return None;
    };
    let host = if let Some(host) = printer.get("host") {
        host
    } else {
        eprintln!("Missing `host` in printer config.");
        return None;
    };
    let password = if let Some(password) = printer.get("password") {
        password
    } else {
        eprintln!("Missing `password` in printer config.");
        return None;
    };
    let name = if let Some(name) = printer.get("name") {
        name
    } else {
        eprintln!("Missing `name` in printer config.");
        return None;
    };
    Some(Printer {
        name: name.to_string(),
        id: dev_id.to_string(),
        ip: host.to_string(),
        model: model.to_string(),
        password: password.to_string(),
    })
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    env_logger::init();

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
                    eprintln!(
                        "No config file found. Try adding one at `{}`",
                        config_file_path.to_string_lossy()
                    );
                }
                ConfigError::FileParse { uri, cause } => {
                    if let Some(uri) = uri {
                        eprintln!("Error parsing config file at {}\nCause:{}", uri, cause);
                    } else {
                        eprintln!("Error parsing config file.")
                    }
                }
                _ => {
                    eprintln!("Unknown error parsing config file {:?}", err)
                }
            }
            return Ok(());
        }
    };
    let addr = config
        .get_string("endpoint")
        .unwrap_or("[::1]:47403".into());
    let addr = addr.parse().unwrap();

    let farm = Farm::default();
    for printer in config.get_array("printers").unwrap_or_default() {
        if let Some(printer) = construct_printer(printer) {
            farm.printers.lock().unwrap().push(printer);
        }
    }
    if farm.printers.lock().unwrap().is_empty() {
        eprintln!(
            "No printers found, or printer config was invalid. Try adding some to the config file at `{}`",
            config_file_path.to_string_lossy()
        );
        return Ok(());
    }

    println!("BambuFarmServer listening on {}", addr);

    Server::builder()
        .add_service(BambuFarmServer::new(farm))
        .serve(addr)
        .await?;

    Ok(())
}
