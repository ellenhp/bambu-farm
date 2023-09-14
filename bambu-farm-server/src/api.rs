use std::collections::HashMap;
use std::fs::File;
use std::io::BufReader;
use std::net::Ipv4Addr;
use std::net::UdpSocket;
use std::pin::Pin;
use std::sync::Arc;
use std::sync::Mutex;
use std::thread;
use std::time::Duration;

use futures::StreamExt;
use lazy_static::lazy_static;
use rumqttc::tokio_rustls::rustls::ClientConfig;
use rumqttc::tokio_rustls::rustls::RootCertStore;
use rumqttc::Client;
use rumqttc::Event;
use rumqttc::EventLoop;
use rumqttc::Incoming;
use rumqttc::MqttOptions;
use rustls_native_certs::Certificate;
use tokio::runtime::{self, Runtime};
use tokio::spawn;
use tokio::time::sleep;

pub fn bambu_network_rs_init() {}

pub fn bambu_network_rs_connect(
    device_id: String,
    device_ip: String,
    username: String,
    password: String,
    ca_path: String,
) -> i32 {
    println!("Attempting connection.");
    let mut mqttoptions = MqttOptions::new(&device_id, &device_ip, 8883);
    mqttoptions.set_transport(rumqttc::Transport::Tls(rumqttc::TlsConfiguration::Rustls(
        Arc::new(
            ClientConfig::builder()
                .with_safe_defaults()
                .with_root_certificates(RootCertStore::empty())
                .with_no_client_auth()
                .into(),
        ),
    )));
    mqttoptions.set_keep_alive(Duration::from_secs(5));

    let (mut client, mut connection) = Client::new(mqttoptions, 10);
    println!("Connected.");
    thread::spawn(move || {
        for (seq, event) in connection.iter().enumerate() {
            match event {
                Ok(Event::Incoming(packet)) => {
                    dbg!(packet);
                }
                other => {
                    dbg!(other);
                }
            }
        }
    });

    println!("Spawned event loop");
    client
        .subscribe(
            format!("device/{}/report", device_id),
            rumqttc::QoS::AtLeastOnce,
        )
        .unwrap();
    println!("Subscribed");
    CONNECTIONS.lock().unwrap().insert(device_id, client);
    println!("Saved connection.");
    0
}

pub fn bambu_network_rs_send(device_id: String, data: String) -> i32 {
    println!("Sending {}", data);
    let mut connections = CONNECTIONS.lock().unwrap();
    let client = connections.get_mut(&device_id).unwrap();
    match client.publish(
        format!("device/{}/request", device_id),
        rumqttc::QoS::AtLeastOnce,
        false,
        data,
    ) {
        Ok(_) => {}
        Err(error) => {
            dbg!(error);
        }
    }
    println!("Done sending");
    0
}
