
.section .sos_sysjmptbl,"ax"
.global  sos_sysjmptbl
sos_sysjmptbl:
jmp ker_sys_fnptr_call     ; 0
jmp ker_sys_malloc         ; 1
jmp ker_sys_realloc        ; 2
jmp ker_sys_free           ; 3
jmp ker_sys_msg_take_data  ; 4
jmp ker_sys_timer_start    ; 5
jmp ker_sys_timer_restart  ; 6
jmp ker_sys_timer_stop     ; 7
jmp ker_sys_post           ; 8
jmp ker_sys_post_link      ; 9
jmp ker_sys_post_value     ; 10
jmp ker_hw_type            ; 11
jmp ker_id                 ; 12
jmp ker_rand               ; 13
jmp ker_systime32          ; 14
jmp ker_sensor_get_data    ; 15
jmp ker_led                ; 16
jmp ker_sys_get_module_state ; 17
jmp ker_sys_fntable_subscribe ; 18
jmp ker_sys_change_own     ; 19
#ifdef SOS_SFI	
jmp ker_sys_fnptr_from_ker_call ; 20
#else
jmp 0                      ; 20
#endif	
jmp ker_sys_codemem_read   ; 21
jmp ker_sys_shm_open       ; 22
jmp ker_sys_shm_update     ; 23
jmp ker_sys_shm_close      ; 24
jmp ker_sys_shm_get        ; 25
jmp ker_sys_shm_wait       ; 26
jmp ker_sys_shm_stopwait   ; 27
jmp ker_get_current_pid    ; 28
jmp ker_get_caller_pid     ; 29
jmp ker_sys_routing_register ; 30
jmp ker_adc_proc_bindPort       ; 31
jmp ker_adc_proc_unbindPort     ; 32
jmp ker_adc_proc_getData        ; 33
jmp ker_sensor_register     ; 34
jmp ker_sensor_deregister   ; 35
jmp ker_sensor_data_ready   ; 36
jmp ker_sensor_enable       ; 37
jmp ker_sensor_disable      ; 38
jmp ker_i2c_reserve_bus     ; 39
jmp ker_i2c_release_bus     ; 40
jmp ker_i2c_send_data       ; 41
jmp ker_i2c_read_data       ; 42
jmp post_longer             ; 43
jmp ker_sys_sensor_start_sampling		; 44
jmp ker_sys_sensor_stop_sampling		; 45
jmp ker_sys_sensor_driver_register		; 46
jmp ker_sys_sensor_driver_deregister	; 47
jmp 0	; jmp ker_sys_adc_bind_channel	; 48
jmp 0	; jmp ker_sys_adc_unbind_channel; 49
jmp 0	; jmp ker_adc_get_data			; 50
jmp 0	; jmp ker_adc_stop_data			; 51
jmp 0	; jmp ker_sys_register_isr		; 52 // For interrupt controller in msp430
jmp 0	; jmp ker_sys_deregister_isr	; 53	
jmp ker_sensor_control                  ; 54
jmp 0                      				; 55
