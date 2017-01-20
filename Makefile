DIRS := BinLoader wrapper.setup revtracer-wrapper revtracer ipclib  DisableSSE  Execution loader ParserPayload execution.inprocess.test libxml-payload
prefix := /usr/local

all:
	for d in $(DIRS); do $(MAKE) clean -C $$d && $(MAKE) -C $$d; done

clean:
	for d in $(DIRS); do $(MAKE) clean -C $$d; done

install:
	for d in $(DIRS); do $(MAKE) clean -C $$d && \
		$(MAKE) -C $$d && \
		$(MAKE) install prefix=$(prefix) -C $$d; done


