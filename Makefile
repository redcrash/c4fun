all:
	$(MAKE) -C mem_alloc
	$(MAKE) -C pebs_tests
	$(MAKE) -C cache_tests
	$(MAKE) -C wsm_ep_cache

clean:
	$(MAKE) -C mem_alloc clean
	$(MAKE) -C pebs_tests clean
	$(MAKE) -C cache_tests clean
	$(MAKE) -C wsm_ep_cache clean
