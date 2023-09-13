.ONESHELL:

shared:
	cargo build
	mkdir -p target/debug/shared
	cd target/debug/shared
	rm * || echo "nothing to remove"
	ar -x ../libbambu_farm_client.a
	gcc -shared *.o -o libbambu_networking.so
	echo "" | gcc -fPIC -shared -x c -o libBambuSource.so -
	rm $$HOME/.config/BambuStudio/plugins/*.so || echo "nothing to remove"
	ln -s $$PWD/libbambu_networking.so $$HOME/.config/BambuStudio/plugins/
	ln -s $$PWD/libBambuSource.so $$HOME/.config/BambuStudio/plugins/
