// english. other language maps will defer to english
// map for any missing key/value pairs
kiri.lang['en'] =
kiri.lang['en-us'] = {
    version:        "version",
    enable:         "enable",

    // DEVICE dialog groups
    dv_gr_dev:      "device",
    dv_gr_ext:      "extruder",
    dv_gr_gco:      "gcode",

    // DEVICE dialog (_s = label, _l = hover help)
    dv_name_s:      "name",
    dv_name_l:      "device name",
    dv_fila_s:      "filament",
    dv_fila_l:      "diameter in millimeters",
    dv_nozl_s:      "nozzle",
    dv_nozl_l:      "diameter in millimeters",
    dv_bedw_s:      "width",
    dv_bedw_l:      "millimeters",
    dv_bedd_s:      "depth",
    dv_bedd_l:      "millimeters",
    dv_bedh_s:      "height",
    dv_bedh_l:      "max build height\nin millimeters",
    dv_spmx_s:      "max spindle",
    dv_spmx_l:      "max spindle rpm speed\n0 to disable",
    dv_xtab_s:      "absolute positioning",
    dv_xtab_l:      "extrusion moves absolute",
    dv_orgc_s:      "origin center",
    dv_orgc_l:      "bed origin center",
    dv_orgt_s:      "origin top",
    dv_orgt_l:      "part z origin top",
    dv_bedc_s:      "circular bed",
    dv_bedc_l:      "device bed is circular",
    dv_fanp_s:      "fan power",
    dv_fanp_l:      "set cooling fan power",
    dv_prog_s:      "progress",
    dv_prog_l:      "output on each % progress",
    dv_layr_s:      "layer",
    dv_layr_l:      "output at each\nlayer change",
    dv_tksp_s:      "token spacing",
    dv_tksp_l:      "gcode token spacer",
    dv_strc_s:      "strip comments",
    dv_strc_l:      "strip gcode comments",
    dv_fext_s:      "file extension",
    dv_fext_l:      "file name extension",
    dv_dwll_s:      "dwell",
    dv_dwll_l:      "gcode dwell script",
    dv_tool_s:      "tool change",
    dv_tool_l:      "tool change script",
    dv_sspd_s:      "spindle speed",
    dv_sspd_l:      "set spindle speed",
    dv_paus_s:      "pause",
    dv_paus_l:      "gcode pause script",
    dv_head_s:      "header",
    dv_head_l:      "gcode header script",
    dv_foot_s:      "footer",
    dv_foot_l:      "gcode footer script",
    dv_lzon_s:      "laser on",
    dv_lzon_l:      "gcode laser on script",
    dv_lzof_s:      "laser off",
    dv_lzof_l:      "gcode laser off script",
    dv_exts_s:      "select",
    dv_exts_l:      "gcode to select this extruder",
    dv_extd_s:      "deselect",
    dv_extd_l:      "gcode to deselect this extruder",
    dv_exox_s:      "offset x",
    dv_exox_l:      "nozzle offset x",
    dv_exoy_s:      "offset y",
    dv_exoy_l:      "nozzle offset y",

    // MODE
    mo_menu:        "mode",
    mo_fdmp:        "FDM Print",
    mo_slap:        "SLA Print",
    mo_lazr:        "Laser Cut",
    mo_cncm:        "CNC Mill",

    // SETUP
    su_menu:        "setup",
    su_devi:        "Devices",
    su_tool:        "Tools",
    su_locl:        "Local",
    su_xprt:        "Export",
    su_help:        "Help",

    // FUNCTION
    fn_menu:        "function",
    fn_impo:        "Import",
    fn_arra:        "Arrange",
    fn_slic:        "Slice",
    fn_prev:        "Preview",
    fn_expo:        "Export",

    // VIEW
    vu_menu:        "view",
    vu_home:        "home",
    vu_rset:        "reset",
    vu_sptp:        "top",
    vu_spfr:        "front",
    vu_splt:        "left",
    vu_sprt:        "right",

    // WORKSPACE
    ws_menu:        "workspace",
    ws_save:        "Save",
    ws_cler:        "Clear",

    // OPTIONS
    op_menu:        "options",
    op_xprt_s:      "expert",
    op_xprt_l:      "enable expert options",
    op_dark_s:      "dark mode",
    op_dark_l:      "dark mode interface",
    op_comp_s:      "compact ui",
    op_comp_l:      "compact user interface\nbetter for small screens\nand tablets",
    op_show_s:      "show origin",
    op_show_l:      "show device or process origin",
    op_alig_s:      "align top",
    op_alig_l:      "align parts to the\ntallest part when\nno stock is set",
    op_auto_s:      "auto layout",
    op_auto_l:      "automatically layout platform\nwhen new items added\nor when arrange clicked\nmore than once",
    op_free_s:      "free layout",
    op_free_l:      "permit dragable layout",
    op_invr_s:      "invert zoom",
    op_invr_l:      "invert mouse wheel\nscroll zoom",
    op_unit_s:      "units",
    op_unit_l:      "workspace units affects\nspeeds and distances",

    // LAYERS pop-menu
    la_menu:        "layers",
    la_olin:        "outline",
    la_trce:        "trace",
    la_face:        "facing",
    la_ruff:        "roughing",
    la_fini:        "finishing",
    la_finx:        "finish x",
    la_finy:        "finish y",
    la_dlta:        "delta",
    la_slds:        "solids",
    la_fill:        "solid fill",
    la_sprs:        "sparse",
    la_sprt:        "support",
    la_prnt:        "print",
    la_move:        "moves",

    // SETTINGS
    se_menu:        "settings",
    se_load:        "load",
    se_save:        "save",

    // FDM SLICING
    sl_menu:        "slice",
    sl_lahi_s:      "layer height",
    sl_lahi_l:      "height of each slice\nlayer in millimeters",
    sl_shel_s:      "shell count",
    sl_shel_l:      "number of perimeter\nwalls to generate",
    sl_ltop_s:      "top layers",
    sl_ltop_l:      "number of solid layers\nto enforce at the\ntop of the print",
    sl_lsld_s:      "solid layers",
    sl_lsld_l:      "solid fill areas computed\nfrom layer deltas. see\nlayer pop menu",
    sl_lbot_s:      "base layers",
    sl_lbot_l:      "number of solid layers\nto enforce at the\nbottom of the print",
    sl_lstp_s:      "stop layer",
    sl_lstp_l:      "where to stop mid-print",

    // FDM FILL
    fi_menu:        "fill",
    fi_type:        "type",
    fi_pcnt_s:      "percentage",
    fi_pcnt_l:      "fill density values\n0.0 - 1.0",
    fi_angl_s:      "start angle",
    fi_angl_l:      "base angle in degrees",
    fi_over_s:      "overlap",
    fi_over_l:      "overlap with shell and fill\nas % of nozzle width\nhigher bonds better\n0.0 - 1.0",

    // FDM FIRST LAYER
    fl_menu:        "first layer",
    fl_lahi_s:      "layer height",
    fl_lahi_l:      "height of each slice\nin millimeters\nshould be >= slice height",
    fl_rate_s:      "shell speed",
    fl_rate_h:      "printing max speed\nin millimeters / minute",
    fl_frat_s:      "fill speed",
    fl_frat_l:      "printing max speed\nin millimeters / minute",
    fl_mult_s:      "print factor",
    fl_mult_l:      "extrusion multiplier\n0.0 - 2.0",
    fl_skrt_s:      "skirt count",
    fl_skrt_l:      "number of first-layer offset\nbrims to generate",
    fl_skro_s:      "skirt offset",
    fl_skro_l:      "skirt offset from part\nin millimeters",
    fl_nozl_s:      "nozzle temp",
    fl_nozl_l:      "degrees in celsius\noutput setting used\nwhen this is zero",
    fl_bedd_s:      "bed temp",
    fl_bedd_l:      "degrees in celsius\noutput setting used\nwhen this is zero",

    // FDM SUPPORT
    sp_menu:        "support",
    sp_dens_s:      "density",
    sp_dens_l:      "percentage 0.0 - 1.0\nrecommended 0.15\n0 to disable",
    sp_size_s:      "pillar size",
    sp_size_l:      "pillar width\nin millimeters",
    sp_offs_s:      "part offset",
    sp_offs_l:      "offset from part\nin millimeters",
    sp_gaps_s:      "gap layers",
    sp_gaps_l:      "number of layers\noffset from part",
    sp_span_s:      "max bridge",
    sp_span_l:      "span length that\ntriggers support\nin millimeters",
    sp_area_s:      "min area",
    sp_area_l:      "minimum area for\na support column\nin millimeters",
    sp_xpnd_s:      "expand",
    sp_xpnd_l:      "expand support area\nbeyond part boundary\nin millimeters",
    sp_nozl_s:      "extruder",
    sp_nozl_l:      "in multi-extruder systems\nthe extruder to use for\nsupport material",

    // LASER SLICING
    ls_offs_s:      "offset",
    ls_offs_l:      "adjust for beam width\nin millimeters",
    ls_lahi_s:      "height",
    ls_lahi_l:      "layer height\nin millimeters\n0 = auto/detect",
    ls_sngl_s:      "single",
    ls_sngl_l:      "perform only one slice\nat specified layer height",

    // CNC COMMON terms
    cc_tool:        "tool",
    cc_spnd_s:      "spindle speed",
    cc_spnd_l:      "spindle speed in\nrevolutions / minute",
    cc_sovr_s:      "step over",
    cc_sovr_l:      "0.1 - 1.0\npercentage of\ntool diameter",
    cc_sdwn_s:      "step down",
    cc_sdwn_l:      "step down depth\nfor each pass\nin workspace units\n0 to disable",
    cc_feed_s:      "feed rate",
    cc_feed_l:      "max cutting speed in\nworkspace units / minute",
    cc_plng_s:      "plunge rate",
    cc_plng_l:      "max z axis speed in\nworkspace units / minute",
    cc_pock_s:      "pocket only",
    cc_pock_l:      "constrain cuts to\npart boundaries",

    // CNC COMMON
    cc_menu:        "common",
    cc_rapd_s:      "rapid feed",
    cc_rapd_l:      "rapid moves feedrate\nin workspace units / minute",

    // CNC ROUGHING
    cr_menu:        "roughing",
    cr_lsto_s:      "leave stock",
    cr_lsto_l:      "horizontal offset from vertical faces\nstock to leave for finishing pass\nin workspace units",
    cr_ease_s:      "ease down",
    cr_ease_l:      "plunge cuts will\nspiral down or ease\nalong a linear path",

    // CNC FINISHING
    cf_menu:        "finishing",
    cf_angl_s:      "max angle",
    cf_angl_l:      "angles greater than this\nare considered vertical",
    cf_watr_s:      "waterline",
    cf_watr_l:      "enable contour finishing\ndisabled when pocketing",
    cf_linx_s:      "linear x",
    cf_linx_l:      "linear x-axis finishing",
    cf_liny_s:      "linear y",
    cf_liny_l:      "linear y-axis finishing",
    cf_curv_s:      "curves only",
    cf_curv_l:      "limit linear cleanup\nto curved surfaces",

    // CNC DRILLING
    cd_menu:        "drilling",
    cd_plpr_s:      "plunge per",
    cd_plpr_l:      "max plunge between\ndwell periods\nin workspace units\n0 to disable",
    cd_dwll_s:      "dwell time",
    cd_dwll_l:      "dwell time\nbetween plunges in\nin milliseconds",
    cd_lift_s:      "drill lift",
    cd_lift_l:      "lift between plunges\nafter dwell period\nin workspace units\n0 to disable",

    // CNC CUTOUT TABS
    ct_menu:        "cutout tabs",
    ct_angl_s:      "angle",
    ct_angl_l:      "starting angle for tab spacing\nin degrees (0-360)",
    ct_numb_s:      "count",
    ct_numb_l:      "number of tabs to use\nwill be spaced evenly\naround the part",
    ct_wdth_s:      "width",
    ct_wdth_l:      "width in workspace units",
    ct_hght_s:      "height",
    ct_hght_l:      "height in workspace units",
    ct_nabl_l:      "enable or disable tabs\ntab generation skipped when\npocket only mode enabled",

    // FDM RAFT
    fr_menu:        "raft",
    fr_spac_s:      "spacing",
    fr_spac_l:      "additional layer spacing\nbetween 1st layer and raft\nin millimeters",
    fr_nabl_l:      "create a raft under the\nmodel for better adhesion\nuses skirt offset and\ndisables skirt output",

    // OUTPUT
    ou_menu:        "output",

    // OUTPUT LASER
    ou_spac_s:      "spacing",
    ou_spac_l:      "distance between layer output\nin millimeters",
    ou_scal_s:      "scaling",
    ou_scal_l:      "multiplier (0.1 to 100)",
    ou_powr_s:      "power",
    ou_powr_l:      "0 - 100\nrepresents %",
    ou_sped_s:      "speed",
    ou_sped_l:      "millimeters / minute",
    ou_mrgd_s:      "merged",
    ou_mrgd_l:      "merge all layers using\ncolor coding to denote\nstacking depth",
    ou_grpd_s:      "grouped",
    ou_grpd_l:      "retain each layer as\na unified grouping\ninstead of separated\npolygons",

    // OUTPUT FDM
    ou_nozl_s:      "nozzle temp",
    ou_nozl_l:      "degrees in celsius",
    ou_bedd_s:      "bed temp",
    ou_bedd_l:      "degrees in celsius",
    ou_feed_s:      "print speed",
    ou_feed_l:      "max print speed\nmillimeters / minute",
    ou_fini_s:      "finish speed",
    ou_fini_l:      "outermost shell speed\nmillimeters / minute",
    ou_move_s:      "move speed",
    ou_move_l:      "non-print move speed\nmillimeters / minute\n0 = enable G0 moves",
    ou_shml_s:      "shell factor",
    ou_flml_s:      "solid factor",
    ou_spml_s:      "infill factor",
    ou_exml_l:      "extrusion multiplier\n0.0 - 2.0",
    ou_fanl_s:      "fan layer",
    ou_fanl_l:      "layer to enable fan",

    // OUTPUT CAM
    ou_toll_s:      "tolerance",
    ou_toll_l:      "surface precision\nin workspace units",
    ou_ztof_s:      "z top offset",
    ou_ztof_l:      "offset from stock surface\nto top face of part\nin workspace units",
    ou_zbot_s:      "z bottom",
    ou_zbot_l:      "offset from part bottom\nto limit cutting depth\nin workspace units",
    ou_zclr_s:      "z clearance",
    ou_zclr_l:      "travel offset from z top\nin workspace units",
    ou_conv_s:      "conventional",
    ou_conv_l:      "milling direction\nuncheck for 'climb'",
    ou_depf_s:      "depth first",
    ou_depf_l:      "optimize pocket cuts\nwith depth priority",

    // CAM STOCK
    cs_menu:        "stock",
    cs_wdth_s:      "width",
    cs_wdth_l:      "width (x) in workspace units\n0 defaults to part size",
    cs_dpth_s:      "depth",
    cs_dpth_l:      "depth (y) in workspace units\n0 defaults to part size",
    cs_hght_s:      "height",
    cs_hght_l:      "height (z) in workspace units\n0 defaults to part size",
    cs_offs_s:      "offset",
    cs_offs_l:      "use width, depth, height\nas offsets from max\npart size on platform",

    // ORIGIN (CAM & LASER)
    or_bnds_s:      "origin bounds",
    or_bnds_l:      "origin is relative to\nboundary of all objects",
    or_cntr_s:      "origin center",
    or_cntr_l:      "origin is referenced from the center",
    or_topp_s:      "origin top",
    or_topp_l:      "origin is references from the top of objects",

    // FDM ADVANCED
    ad_menu:        "advanced",
    ad_rdst_s:      "retract dist",
    ad_rdst_l:      "amount to retract filament\nfor long moves. in millimeters",
    ad_rrat_s:      "retract rate",
    ad_rrat_l:      "speed of filament\nretraction in mm/s",
    ad_rdwl_s:      "engage dwell",
    ad_rdwl_l:      "time between re-engaging\nfilament and movement\nin milliseconds",
    ad_scst_s:      "shell coast",
    ad_scst_l:      "non-printing end\nof perimeter shells\nin millimeters",
    ad_msol_s:      "min solid",
    ad_msol_l:      "minimum area (mm^2)\nrequired to keep solid\nmust be > 0.1",
    ad_minl_s:      "min layer",
    ad_minl_l:      "enables adaptive slicing with\nthis as the min layer height\nin millimeters\n0 to disable",
    ad_mins_s:      "min speed",
    ad_mins_l:      "minimum speed\nfor short segments",
    ad_spol_s:      "slow poly",
    ad_spol_l:      "polygons shorter than this\nwill have their print speed\nscaled down to min speed\nin millimeters",
    ad_zhop_s:      "z hop dist",
    ad_zhop_l:      "amount to raise z\non retraction moves\nin millimeters\n0 to disable",
    ad_abkl_s:      "anti-backlash",
    ad_abkl_l:      "use micro-movements to cancel\nbacklash during fills\nin millimeters",
    ad_slrt_s:      "slice rotation",
    ad_slrt_l:      "slice object on a bias\nangle in the y axis.\nuseful for extremely\nlarge format prints",
    ad_lret_s:      "layer retract",
    ad_lret_l:      "force filament retraction\nbetween layers",
    ad_play_s:      "polish layers",
    ad_play_l:      "polish up to specified\n# of layers at a time",
    ad_pspd_s:      "polish speed",
    ad_pspd_l:      "polishing speed\nin millimeters / minute",

    // FDM GCODE
    ag_menu:        "gcode",
    ag_nozl_s:      "nozzle",
    ag_nozl_l:      "select output nozzle or head",
    ag_paws_s:      "pause layers",
    ag_paws_l:      "comma-separated list of layers\nto inject pause commands before",

    // SLA MENU
    sa_menu:        "slice",
    sa_lahe_s:      "layer height",
    sa_lahe_l:      "layer height\nin millimeters",
    sa_shel_s:      "hollow shell",
    sa_shel_l:      "shell thickness in mm\nuse multiple of layer height\nuse 0 for solid (disabled)",
    sa_otop_s:      "open top",
    sa_otop_l:      "if shell is enabled\nresults in an open top",
    sa_obas_s:      "open base",
    sa_obas_l:      "if shell is enabled\nresults in an open base\ndisabled with supports",

    sa_layr_m:      "layers",
    sa_lton_s:      "light on time",
    sa_lton_l:      "layer light on\ntime in seconds",
    sa_ltof_s:      "light off time",
    sa_ltof_l:      "layer light off\ntime in seconds",
    sa_pldi_s:      "peel distance",
    sa_pldi_l:      "peel distance\nin millimeters",
    sa_pllr_s:      "peel lift rate",
    sa_pllr_l:      "peel lift speed\nin mm/sec",
    sa_pldr_s:      "peel drop rate",
    sa_pldr_l:      "peel drop speed\nin mm/sec",

    sa_base_m:      "base layers",
    sa_balc_s:      "layer count",
    sa_balc_l:      "number of\nbase layers",
    sa_bltn_l:      "base layer light on\ntime in seconds",
    sa_bltf_l:      "base layer light off\ntime in seconds",

    sa_infl_m:      "infill",
    sa_ifdn_s:      "density",
    sa_ifdn_l:      "percent infill\nrequires shell\n0 = disabled\nvalid 0.0 - 1.0",
    sa_iflw_s:      "line width",
    sa_iflw_l:      "hatch line width\nin millimeters",

    sa_supp_m:      "support",
    sa_slyr_s:      "base layers",
    sa_slyr_l:      "base support layers\nvalue range 0-10",
    sa_slgp_s:      "gap layers",
    sa_slgp_l:      "number of layers between\nraft and bottom of object",
    sa_sldn_s:      "density",
    sa_sldn_l:      "used to compute the\nnumber of support pillars\n0.0-1.0 (0 = disable)",
    sa_slsz_s:      "size",
    sa_slsz_l:      "max size of a\nsupport pillar\nin millimeters",
    sa_slpt_s:      "points",
    sa_slpt_l:      "number of points in\neach support pillar\nin millimeters",
    sl_slen_l:      "enable supports",

    sa_outp_m:      "output",
    sa_opzo_s:      "z offset",
    sa_opzo_l:      "z layer offset\nalmost always 0.0\n0.0-1.0 in millimeters",
    sa_opaa_s:      "anti alias",
    sa_opaa_l:      "enable anti-aliasing\nproduces larger files\ncan blur details"
};
