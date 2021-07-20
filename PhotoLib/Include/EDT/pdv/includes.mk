LIBOBJS= \
     $(CFGNAME)/clsim_lib.obj \
     $(CFGNAME)/cl_logic_lib.obj \
     $(CFGNAME)/edt_bitload.obj \
     $(CFGNAME)/edt_error.obj \
     $(CFGNAME)/edt_trace.obj \
     $(CFGNAME)/edt_vco.obj \
     $(CFGNAME)/edt_flash.obj \
     $(CFGNAME)/libdvu.obj \
     $(CFGNAME)/libedt.obj \
     $(CFGNAME)/libpdv.obj \
     $(CFGNAME)/pciload_fn.obj \
     $(CFGNAME)/pdv_bayer_filter.obj \
     $(CFGNAME)/pdv_dmy_image.obj \
     $(CFGNAME)/pdv_initcam.obj \
     $(CFGNAME)/pdv_interlace.obj \
     $(CFGNAME)/pdv_recplay.obj \
     $(CFGNAME)/readcfg.obj \
     $(CFGNAME)/libedt_timing.obj \
     $(CFGNAME)/pdv_irig.obj \
     $(CFGNAME)/edt_si570.obj \
     $(CFGNAME)/lib_two_wire.obj \
     $(CFGNAME)/edt_optstring.obj \
     $(CFGNAME)/chkbuf.obj \
     $(CFGNAME)/edt_si5338.obj \
     $(CFGNAME)/edt_os_nt.obj

TARGETS= \
    bitload.exe \
    checkcam.exe \
    check_gap_cl.exe \
    cl_logic.exe \
    clsiminit.exe \
    cl_speed.exe \
    clink_tester.exe \
    countbits.exe \
    dumpit.exe \
    dvinfo.exe \
    edt_sysinfo.exe \
    gen_tiffs.exe \
    initcam.exe \
    irigdump.exe \
    new_strobe.exe \
    optstr.exe \
    pdb.exe \
    pdbcls10b.exe \
    pdvplayback.exe \
    rcxload.exe \
    rcxmake.exe \
    send_tiffs.exe \
    serial_cmd.exe \
    setdebug.exe \
    setperiod.exe \
    simple_bc.exe \
    simple_clsend.exe \
    simple_event.exe \
    simple_etrig.exe \
    simple_irig2.exe \
    simplemem.exe \
    simple_sequence.exe \
    simplest_take.exe \
    simple_take.exe \
    speedtest.exe \
    strobe.exe \
    take.exe \
    taketwo.exe \
    trace.exe \
    usps_test.exe \
    waitevent.exe \
    watchstat.exe \
    xtest.exe \
    timing_test.exe \
    msp430_load.exe \
    clink_counters.exe \
    pdvrecord.exe \
    bigread.exe \
    clsimvar.exe \
    pdvterm.exe \
    sample.exe \
    set_si5338.exe \
    win_sysinfo.exe

CPP_LIBOBJS=

CPP_TARGETS= \
    pdvcom.exe

