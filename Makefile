
all:
	./mythos/3rdparty/mcconf/mcconf.py -i mythos-amd64.config
	./mythos/3rdparty/mcconf/mcconf.py -i linux.config

clean:
	rm -f *.log
	rm -rf prothos-amd64
