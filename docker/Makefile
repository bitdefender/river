version=0.1

build:
	docker image build -t river:${version} --build-arg VERSION=${version} .

test: build
	docker run --rm --name river3 river:${version}

run: build
	docker run -it --name river3 river:${version} /bin/bash

bash:
	docker exec -it river3 /bin/bash

clean:
	docker rm -f river3
	docker image rm river:${version}
