
all:
	cd bambu-farm-client && make shared
	cd bambu-farm-server && cargo build

run: all
	cd bambu-farm-server && RUST_LOG=warn cargo run

