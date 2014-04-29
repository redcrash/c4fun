all:
	$(MAKE) -C mem_alloc
	$(MAKE) -C cache_tests
	$(MAKE) -C mem_load
	$(MAKE) -C mem_model
	$(MAKE) -C pebs_tests
	$(MAKE) -C perf_event_open_tests
	$(MAKE) -C pmu_msr	
clean:
	$(MAKE) -C cache_tests clean
	$(MAKE) -C mem_alloc clean
	$(MAKE) -C mem_load clean
	$(MAKE) -C mem_model clean
	$(MAKE) -C pebs_tests clean
	$(MAKE) -C perf_event_open_tests clean
	$(MAKE) -C pmu_msr clean
