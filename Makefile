
all:
	./mythos/3rdparty/mcconf/mcconf.py -i prothos-amd64.config

clean:
	rm -f *.log
	rm -rf prothos-amd64
