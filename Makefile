DIRS := revtracer-wrapper revtracer Execution ParserPayload DisableSSE execution.inprocess.test tracer.simple
prefix := /usr/local

all:
	for d in $(DIRS); do $(MAKE) clean -C $$d && $(MAKE) -C $$d; done

clean:
	for d in $(DIRS); do $(MAKE) clean -C $$d; done

install:
	for d in $(DIRS); do $(MAKE) clean -C $$d && \
		$(MAKE) -C $$d && \
		$(MAKE) install prefix=$(prefix) -C $$d; done


