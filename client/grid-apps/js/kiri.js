/** Copyright Stewart Allen -- All Rights Reserved */
"use strict";

self.kiri = (self.kiri || {});
self.kiri.version = exports.VERSION;
self.kiri.license = exports.LICENSE;
self.kiri.copyright = exports.COPYRIGHT;

(function () {

    let iOS = /(iPad|iPhone|iPod)/g.test(navigator.userAgent),
        autoDecimate = true,
        // ---------------
        MOTO    = moto,
        KIRI    = self.kiri,
        BASE    = self.base,
        UTIL    = BASE.util,
        DBUG    = BASE.debug,
        LANG    = KIRI.lang.current,
        WIN     = self.window,
        DOC     = self.document,
        LOC     = self.location,
        HOST    = LOC.host.split(':'),
        SETUP   = parseOpt(LOC.search.substring(1)),
        SECURE  = isSecure(LOC.protocol),
        LOCAL   = self.debug && !SETUP.remote,
        SDB     = MOTO.KV,
        ODB     = KIRI.odb = new MOTO.Storage(SETUP.d ? SETUP.d[0] : 'kiri'),
        SPACE   = KIRI.space = MOTO.Space,
        WIDGETS = KIRI.widgets = [],
        SCRAPS  = KIRI.scraps = [],
        SCRAPSMOVE = KIRI.scrapsMove = [{x: 0, y: 0, z: 0}], // LWW TODO: Use this as a list instead of just [0] for filling multiple models with scrap 
        CATALOG = KIRI.catalog = KIRI.openCatalog(ODB,autoDecimate),
        STATS   = new Stats(SDB),
        SEED    = 'kiri-seed',
        // ---------------
        CONF    = KIRI.conf,
        MODES   = CONF.MODES,
        VIEWS   = CONF.VIEWS,
        clone   = Object.clone,
        settings = clone(CONF.template),
        settingsDefault = clone(settings),
        // ---------------
        Widget    = kiri.Widget,
        newWidget = kiri.newWidget,
        // ---------------
        UI = {},
        UC = MOTO.ui.prefix('kiri').inputAction(updateSettings).hideAction(updateDialogLeft),
        MODE = MODES.FDM,
        onEvent = {},
        currentPrint = null,
        selectedMeshes = [],
        localFilterKey ='kiri-gcode-filters',
        localFilters = js2o(SDB.getItem(localFilterKey)) || [],
        // ---------------
        renderMode = 4,
        viewMode = VIEWS.ARRANGE,
        layoutOnAdd = true,
        local = SETUP.local,
        camStock = null,
        camTopZ = 0,
        topZ = 0,
        showFavorites = SDB.getItem('dev-favorites') === 'true',
        alerts = [],
        grouping = false,
        // ---------------
        slicer = KIRI.slicer;

    if (SETUP.rm) renderMode = parseInt(SETUP.rm[0]);
    DBUG.enable();

    // add show() to catalog for API
    CATALOG.show = showCatalog;

    const feature = {
        seed: true,
        controls: true
    };

    const selection = {
        opacity: setOpacity,
        move: moveSelection,
        scale: scaleSelection,
        rotate: rotateSelection,
        meshes: function() { return selectedMeshes.slice() },
        widgets: function() { return selectedMeshes.slice().map(m => m.widget) },
        for_groups: forSelectedGroups,
        for_meshes: forSelectedMeshes,
        for_widgets: forSelectedWidgets
    };

    const platform = {
        add: platformAdd,
        delete: platformDelete,
        layout: platformLayout,
        load: platformLoad,
        load_stl: platformLoadSTL,
        deselect: platformDeselect,
        select: platformSelect,
        select_all: platformSelectAll,
        selected_count: platformSelectedCount,
        compute_max_z: platformComputeMaxZ,
        update_origin: platformUpdateOrigin,
        update_bounds: platformUpdateBounds,
        update_stock: platformUpdateStock,
        update_size: platformUpdateSize,
        update_top_z: platformUpdateTopZ,
        update_selected: platformUpdateSelected,
        load_files: platformLoadFiles,
        group: platformGroup,
        group_done: platformGroupDone
    };

    const color = {
        wireframe: 0x444444,
        wireframe_opacity: 0.25,
        selected: [ 0xbbff00, 0xbbee00, 0xbbdd00 ],
        deselected: [ 0xffff00, 0xffdd00, 0xffbb00 ],
        slicing: 0xffaaaa,
        preview_opacity: 0.0,
        model_opacity: 1.0,
        slicing_opacity: 0.5,
        sliced_opacity: 0.0,
        cam_preview: 0x0055bb,
        cam_preview_opacity: 0.25,
        cam_sliced_opacity: 0.25
    };

    const lists = {
        infill: [
            { name: "vase" },
            { name: "hex" },
            { name: "grid" },
            { name: "gyroid" },
            { name: "triangle" },
            { name: "linear" },
            { name: "bubbles2" },
            { name: "bubbles" }
        ],
        units: [
            { name: "mm" },
            { name: "in" }
        ],
        antialias: [
            { name: "1", id: 1 },
            { name: "2", id: 2 },
            { name: "4", id: 4 },
            { name: "8", id: 8 }
        ]
    };

    const API = KIRI.api = {
        ui: UI,
        uc: UC,
        sdb: SDB,
        o2js: o2js,
        js2o: js2o,
        ajax: ajax,
        clone: clone,
        focus: setFocus,
        stats: STATS,
        catalog: CATALOG,
        conf: {
            get: getSettings,
            put: putSettings,
            load: loadSettings,
            save: saveSettings,
            show: showSettings,
            update: updateSettings,
            restore: restoreSettings,
            export: settingsExport,
            import: settingsImport
        },
        color,
        const: {
            SEED,
            LANG,
            LOCAL,
            MODES,
            VIEWS,
            SETUP
        },
        var: {
            layer_at: Infinity,
            layer_max: 0,
            layer_range: 0
        },
        device: {
            get: currentDeviceName,
            set: undefined // set during init
        },
        dialog: {
            show: showDialog,
            hide: hideDialog,
            update: updateDialogLeft
        },
        help: {
            show: showHelp,
            file: showHelpFile
        },
        event: {
            on: addOnEvent,
            emit: sendOnEvent,
            import: loadFile,
            alerts: updateAlerts,
            settings: triggerSettingsEvent
        },
        feature,
        function: {
            slice: prepareSlices,
            print: preparePrint,
            export: function() { KIRI.export() },
            clear: clearWidgetCache
        },
        hide: {
            import: function() { UI.import.style.display = 'none' }
        },
        language: KIRI.lang,
        lists,
        modal: {
            show: showModal,
            hide: hideModal,
            visible: modalShowing
        },
        mode: {
            get_lower: getModeLower,
            get_id: function() { return MODE },
            get: getMode,
            set: setMode,
            switch: switchMode,
            set_expert: setExpert
        },
        print: {
            get: function() { return currentPrint },
            clear: clearPrint
        },
        probe: {
            grid : function() { return false },
            local : function() { return false }
        },
        platform,
        selection,
        show: {
            alert: alert2,
            devices: undefined, // set during init
            progress: setProgress,
            controls: setControlsVisible,
            favorites: getShowFavorites,
            slices: showSlices,
            layer: setVisibleLayer,
            local: showLocal,
            import: function() { UI.import.style.display = '' }
        },
        space: {
            reload: reload,
            restore: restoreWorkspace,
            clear: clearWorkspace,
            save: saveWorkspace,
        },
        view: {
            get: function() { return viewMode },
            set: setViewMode,
            update_fields: updateFields,
            wireframe: toggleWireframe,
            snapshot: null
        },
        widgets: {
            new: newWidget,
            all: function() { return WIDGETS.slice() },
            for: forAllWidgets,
            load: Widget.loadFromCatalog,
            meshes: meshArray,
            opacity: setOpacity
        },
        work: KIRI.work
    };

    function reload() {
        API.event.emit('reload');
        // allow time for async saves to complete
        setTimeout(() => { LOC.reload() }, 50);
    }

    /** ******************************************************************
     * Stats accumulator
     ******************************************************************* */

    function Stats(db) {
        this.db = db;
        this.obj = js2o(this.db['stats'] || '{}');
        let o = this.obj, k;
        for (k in o) {
            if (!o.hasOwnProperty(k)) continue;
            if (k === 'dn' || k.indexOf('-') > 0 || k.indexOf('_') > 0) {
                delete o[k];
            }
        }
    }

    Stats.prototype.save = function(quiet) {
        this.db['stats'] = o2js(this.obj);
        if (!quiet) {
            API.event.emit('stats', this.obj);
        }
        return this;
    };

    Stats.prototype.get = function(k) {
        return this.obj[k];
    };

    Stats.prototype.set = function(k,v,quiet) {
        this.obj[k] = v;
        this.save(quiet);
        return this;
    };

    Stats.prototype.add = function(k,v,quiet) {
        this.obj[k] = (this.obj[k] || 0) + (v || 1);
        this.save(quiet);
        return this;
    };

    Stats.prototype.del = function(k, quiet) {
        delete this.obj[k];
        this.save(quiet);
        return this;
    };

    let inits = parseInt(SDB.getItem('kiri-init') || STATS.get('init') || 0) + 1;
    SDB.setItem('kiri-init', inits);

    STATS.set('init', inits);
    STATS.set('kiri', kiri.version);

    /** ******************************************************************
     * Utility Functions
     ******************************************************************* */

     function unitScale() {
         return MODE === MODES.CAM &&
            settings.controller.units === 'in' ? 25.4 : 1;
     }

     function alert2(message, time) {
         if (message === undefined) {
             return updateAlerts(true);
         }
         alerts.push([message, Date.now(), time]);
         updateAlerts();
     }

     function updateAlerts(clear) {
         if (clear) {
             alerts = [];
         }
         let now = Date.now();
         // filter out by age
         alerts = alerts.filter(alert => {
             return (now - alert[1]) < ((alert[2] || 5) * 1000);
         });
         // limit to 5 showing
         while (alerts.length > 5) {
             alerts.shift();
         }
         // return if called before UI configured
         if (!UI.alert) {
             return;
         }
         if (alerts.length > 0) {
             UI.alert.text.innerHTML = alerts.map(v => ['<p>',v[0],'</p>'].join('')).join('');
             UI.alert.dialog.style.display = 'flex';
         } else {
             UI.alert.dialog.style.display = 'none';
         }
     }

     function getShowFavorites(bool) {
         if (bool !== undefined) {
             SDB.setItem('dev-favorites', bool);
             showFavorites = bool;
             return bool;
         }
         return showFavorites;
     }

     function sendOnEvent(name, data) {
         if (name && onEvent[name]) onEvent[name].forEach(function(fn) {
             fn(data);
         });
     }

     function addOnEvent(name, handler) {
         if (name && typeof(name) === 'string' && typeof(handler) === 'function') {
             onEvent[name] = onEvent[name] || [];
             onEvent[name].push(handler);
         }
     }

     function triggerSettingsEvent() {
         API.event.emit('settings', settings);
     }

    function isSecure(proto) {
         return proto.toLowerCase().indexOf("https") === 0;
    }

    function parseOpt(ov) {
        let opt = {}, kv, kva;
        // handle kiri legacy and proper url encoding better
        ov.replace(/&/g,',').split(',').forEach(function(el) {
            kv = decodeURIComponent(el).split(':');
            if (kv.length === 2) {
                kva = opt[kv[0]] = opt[kv[0]] || [];
                kva.push(decodeURIComponent(kv[1]));
            }
        });
        return opt;
    }

    function ajax(url, fn, rt, po, hd) {
        return new MOTO.Ajax(fn, rt).request(url, po, hd);
    }

    function o2js(o,def) {
        return o ? JSON.stringify(o) : def || null;
    }

    function js2o(s,def) {
        try {
            return s ? JSON.parse(s) : def || null;
        } catch (e) {
            console.log({malformed_json:s});
            return def || null;
        }
    }

    function ls2o(key,def) {
        return js2o(SDB.getItem(key),def);
    }

    function setProgress(value, msg) {
        if (value) {
            value = UTIL.round(value*100,4);
            UI.loading.display = 'block';
            UI.progress.width = value+'%';
            if (msg) UI.prostatus.innerHTML = msg;
        } else {
            UI.loading.display = 'none';
        }
    }

    function bound(v,min,max) {
        return Math.max(min,Math.min(max,v));
    }

    function setVisibleLayer(v) {
        showSlices(API.var.layer_at = bound(v, 0, API.var.layer_max));
    }

    function meshArray() {
        let out = [];
        forAllWidgets(function(widget) {
            out.push(widget.mesh);
        });
        return out;
    }

    function forAllWidgets(f) {
        WIDGETS.slice().forEach(function(widget) {
            f(widget);
        });
        // SCRAPS.slice().forEach(function(scrap) {
        //     f(scrap);
        // });
    }

    function forAllScraps(f) {
        SCRAPS.slice().forEach(function(scrap) {
            f(scrap);
        });
    }

    function forSelectedGroups(f) {
        let m = selectedMeshes;
        if (m.length === 0 && WIDGETS.length === 1) m = [ WIDGETS[0].mesh ];
        let v = [];
        m.slice().forEach(function (mesh) {
            if (v.indexOf(mesh.widget.group) < 0) f(mesh.widget);
            v.push(mesh.widget.group);
        });
    }

    function forSelectedWidgets(f) {
        let m = selectedMeshes;
        if (m.length === 0 && WIDGETS.length === 1) m = [ WIDGETS[0].mesh ];
        m.slice().forEach(function (mesh) { f(mesh.widget) });
    }

    function forSelectedMeshes(f) {
        selectedMeshes.slice().forEach(function (mesh) { f(mesh) });
    }

    function toggleWireframe(color, opacity) {
        forAllWidgets(function(w) { w.toggleWireframe(color, opacity) });
        SPACE.update();
    }

    function updateSliderMax(set) {
        let max = 0;
        if (viewMode === VIEWS.PREVIEW && currentPrint) {
            max = currentPrint.getLayerCount();
        } else {
            forAllWidgets(function(widget) {
                if (!widget.slices) return;
                max = Math.max(max, widget.slices.length);
            });
        }
        max = Math.max(0, max - 1);
        API.var.layer_max = max;
        if (UI.layerID.convert() > max || API.var.layer_at > max) {
            API.var.layer_at = max;
            UI.layerID.value = max;
            UI.layerSlider.value = API.var.layer_max;
        }
        UI.layerSlider.max = max;
        if (set) {
            API.var.layer_at = API.var.layer_max;
            UI.layerSlider.value = API.var.layer_max;
        }
    }

    function hideSlices() {
        let showing = false;
        setOpacity(color.model_opacity);
        forAllWidgets(function(widget) {
            widget.setWireframe(false);
            showing = widget.hideSlices() || showing;
        });
        clearPrint();
        return showing;
    }

    function showSlice(index, range, layer) {
        if (range) {
            return index <= layer && index > layer-range;
        } else {
            return index <= layer;
        }
    }

    /**
     * hide or show slice-layers and their sub-elements
     *
     * @param {number} [layer]
     */
    function showSlices(layer) {
        if (typeof(layer) === 'string' || typeof(layer) === 'number') {
            layer = parseInt(layer);
        } else {
            layer = API.var.layer_at;
        }

        layer = bound(layer, 0, API.var.layer_max);

        UI.layerID.value = layer;
        UI.layerSlider.value = layer;

        let j,
            slice,
            slices,
            layers,
            range = UI.layerRange.checked ? UI.layerSpan.convert() || 1 : 0,
            print = UI.layerPrint.checked,
            moves = UI.layerMoves.checked;

        if (MODE === MODES.CAM && API.var.layer_range !== range && range && layer === API.var.layer_max) {
            layer = 0;
        }

        API.var.layer_range = range;
        API.var.layer_at = layer;

        forAllWidgets(function(widget) {
            if (print) return widget.hideSlices();

            slices = widget.slices;
            if (!slices) return;

            for (j = 0; j < slices.length; j++) {
                slice = slices[j];
                slice.view.visible = showSlice(j, range, layer);
                layers = slice.layers;
                layers.outline.setVisible(
                    MODE === MODES.CAM ?
                        UI.layerOutline.checked && LOCAL :
                        UI.layerOutline.checked
                );
                layers.trace.setVisible(
                    MODE === MODES.CAM ?
                        UI.layerRough.checked :
                        UI.layerTrace.checked
                );
                layers.bridge.setVisible(
                    MODE === MODES.CAM ?
                        UI.layerFinishX.checked :
                        UI.layerDelta.checked
                );
                layers.flat.setVisible(
                    MODE === MODES.CAM ?
                        UI.layerFinishY.checked :
                        UI.layerDelta.checked
                );
                layers.solid.setVisible(
                    MODE === MODES.CAM ?
                        UI.layerFinish.checked :
                        UI.layerSolid.checked
                );
                layers.fill.setVisible(
                    MODE === MODES.CAM ?
                        UI.layerFacing.checked :
                        UI.layerFill.checked
                );
                layers.sparse.setVisible(UI.layerSparse.checked);
                layers.support.setVisible(UI.layerSupport.checked);
            }
        });

        if (currentPrint) {
            let len = currentPrint.getLayerCount();
            for (j = 0; j < len; j++) {
                currentPrint.showLayer(j, print && showSlice(j, range, layer), moves);
            }
        }
        UI.layerPrint.parentNode.style.display = currentPrint ? '' : 'none';
        UI.layerMoves.parentNode.style.display = currentPrint ? '' : 'none';

        SPACE.update();
    }

    function loadCode(code, type) {
        setViewMode(VIEWS.PREVIEW);
        clearPrint();
        setOpacity(0);
        currentPrint = kiri.newPrint(settings, []);
        let center = settings.process.outputOriginCenter;
        let origin = settings.origin;
        let offset = {
            x: origin.x,
            y: -origin.y,
            z: origin.z
        };
        switch (type) {
            case 'svg':
                currentPrint.parseSVG(code, offset);
                break;
            default:
                currentPrint.parseGCode(code, offset);
                break;
        }
        currentPrint.render();
        SPACE.platform.add(currentPrint.group);
        SPACE.update();
        UI.layerPrint.checked = true;
        updateSliderMax(true);
        showSlices();
    }

    function preparePrint(callback) {
        if (viewMode === VIEWS.PREVIEW) return;

        // kick off slicing it hasn't been done already
        for (let i=0; i < WIDGETS.length; i++) {
            if (!WIDGETS[i].slices || WIDGETS[i].isModified()) {
                prepareSlices(function() {
                    if (!WIDGETS[i].slices || WIDGETS[i].isModified()) {
                        alert2("nothing to print");
                    } else {
                        preparePrint(callback);
                    }
                });
                return;
            }
        }

        setViewMode(VIEWS.PREVIEW);
        clearPrint();
        API.conf.save();

        if (MODE === MODES.CAM) {
            setOpacity(color.cam_preview_opacity);
            forAllWidgets(function(widget) {
                widget.setColor(color.cam_preview);
            });
        } else {
            setOpacity(color.preview_opacity);
        }

        currentPrint = kiri.newPrint(settings, WIDGETS);
        currentPrint.setup(true, function(update, status) {
            setProgress(update, status);
        }, function() {
            if (!currentPrint) {
                return setViewMode(VIEWS.ARRANGE);
            }

            setProgress(0);
            setOpacity(0);

            currentPrint.render();

            API.event.emit('print', getMode());
            SPACE.platform.add(currentPrint.group);
            SPACE.update();

            UI.layerPrint.checked = true;
            updateSliderMax(true);
            showSlices();

            if (typeof(callback) === 'function') {
                callback();
            }
        });
    }

    function clearWidgetCache() {
        hideSlices();
        clearSlices();
        clearPrint();
    }

    function clearPrint() {
        if (currentPrint) {
            SPACE.platform.remove(currentPrint.group);
            currentPrint = null;
        }
        UI.layerPrint.checked = false;
    }

    function clearSlices() {
        forAllWidgets(function(widget) {
            widget.slices = null;
        });
    }

    /**
     * incrementally slice all meshes then incrementally update them
     *
     * @param {Function} callback
     */
    function prepareSlices(callback) {
        // console.log("prepareSlices");
        var extractedData = slicer.prepareScrap(SCRAPS, Widget);
        SCRAPS[SCRAPS.length-3] = extractedData;
        for (let widgetIndex = 0; widgetIndex < KIRI.widgets.length; widgetIndex++) {
                            
            KIRI.widgets[widgetIndex].isScrap = true;
            KIRI.widgets[widgetIndex].scrapDataArray = SCRAPS;

            // console.log(KIRI.widgets[widgetIndex].id);
            // console.log(KIRI.widgets[widgetIndex].ScrapDataArray);
            // console.log(KIRI.widgets[widgetIndex].isScrap);
        }

        

        if (viewMode == VIEWS.ARRANGE) {
            let snap = SPACE.screenshot();
            API.view.snapshot = snap.substring(snap.indexOf(",")+1);
            KIRI.work.snap(API.view.snapshot);
        }

        clearPrint();
        platform.deselect();
        setViewMode(VIEWS.SLICE);
        API.conf.save();

        let firstMesh = true,
            countdown = WIDGETS.length,
            preserveMax = API.var.layer_max,
            preserveLayer = API.var.layer_at,
            totalProgress,
            track = {},
            now = UTIL.time();

        // require topo be sent back from worker for local printing
        settings.synth.sendTopo = false;

        setOpacity(color.slicing_opacity);

        // for each widget, slice
        forAllWidgets(function(widget) {
            let segtimes = {},
                segNumber = 0,
                errored = false,
                startTime,
                lastMsg;

            widget.stats.progress = 0;
            widget.setColor(color.slicing);
            widget.slice(settings, function(sliced, error) {
                let mark = UTIL.time();
                // on done
                widget.render(renderMode, MODE === MODES.CAM);
                // clear wireframe
                widget.setWireframe(false, color.wireframe, color.wireframe_opacity);
                widget.setOpacity(settings.mode === 'CAM' ? color.cam_sliced_opacity : color.sliced_opacity);
                widget.setColor(color.deselected);
                // update UI info
                if (sliced) {
                    // update segment time
                    if (lastMsg) segtimes[segNumber+"_"+lastMsg] = mark - startTime;
                    segtimes.total = UTIL.time() - now;
                    DBUG.log(segtimes);
                    API.event.emit('slice', getMode());
                    updateSliderMax(true);
                    if (preserveMax != API.var.layer_max) {
                        preserveLayer = API.var.layer_max;
                    }
                    firstMesh = false;
                }
                // on the last exit, update ui and call the callback
                if (--countdown === 0 || error || errored) {
                    setProgress(0);
                    showSlices(preserveLayer);
                    setOpacity(settings.mode === 'CAM' ? color.cam_sliced_opacity : color.sliced_opacity);
                    if (callback && typeof callback === 'function') {
                        callback();
                    }
                }
                // update slider window
                API.dialog.update();
                // handle slicing errors
                if (error && !errored) {
                    errored = true;
                    setViewMode(VIEWS.ARRANGE);
                    setOpacity(color.model_opacity);
                    platform.deselect();
                    alert2(error);
                }
            }, function(update, msg) {
                if (msg !== lastMsg) {
                    let mark = UTIL.time();
                    if (lastMsg) segtimes[segNumber+"_"+lastMsg] = mark - startTime;
                    lastMsg = msg;
                    startTime = mark;
                    segNumber++;
                }
                // on update
                track[widget.id] = update;
                totalProgress = 0;
                forAllWidgets(function(w) {
                    totalProgress += (track[w.id] || 0);
                });
                setProgress((totalProgress / WIDGETS.length), msg);
            }, true, SCRAPS);
        });
    }

    /** ******************************************************************
     * Selection Functions
     ******************************************************************* */

    function updateSelectedInfo() {
        let bounds = new THREE.Box3(), track;
        forSelectedMeshes(mesh => {
            bounds = bounds.union(mesh.getBoundingBox());
            track = mesh.widget.track;
        });
        if (bounds.min.x === Infinity) {
            if (selectedMeshes.length === 0) {
                UI.sizeX.value = 0;
                UI.sizeY.value = 0;
                UI.sizeZ.value = 0;
                UI.scaleX.value = 1;
                UI.scaleY.value = 1;
                UI.scaleZ.value = 1;
            }
            return;
        }
        let dx = bounds.max.x - bounds.min.x,
            dy = bounds.max.y - bounds.min.y,
            dz = bounds.max.z - bounds.min.z,
            scale = unitScale();
        UI.sizeX.value = UI.sizeX.was = UTIL.round(dx/scale,2);
        UI.sizeY.value = UI.sizeY.was = UTIL.round(dy/scale,2);
        UI.sizeZ.value = UI.sizeZ.was = UTIL.round(dz/scale,2);
        UI.scaleX.value = UI.scaleX.was = track.scale.x;
        UI.scaleY.value = UI.scaleY.was = track.scale.y;
        UI.scaleZ.value = UI.scaleZ.was = track.scale.z;
    }

    function setOpacity(value) {
        forAllWidgets(function (w) { w.setOpacity(value) });
        UI.modelOpacity.value = value * 100;
        SPACE.update();
    }

    function moveSelection(x, y, z, abs) {
        forSelectedGroups(function (w) { w.move(x, y, z, abs) });
        platform.update_stock();
        SPACE.update();
    }

    function scaleSelection() {
        let args = arguments;
        forSelectedGroups(function (w) {
            w.scale(...args);
        });
        // skip update if last argument is strictly 'false'
        if ([...arguments].pop() === false) {
            return;
        }
        updateSelectedInfo();
        platform.compute_max_z();
        platform.update_stock(true);
        SPACE.update();
    }

    function rotateSelection(x, y, z) {
        forSelectedGroups(function (w) {
            w.rotate(x, y, z);
        });
        updateSelectedInfo();
        platform.compute_max_z();
        platform.update_stock(true);
        SPACE.update();
    }

    /** ******************************************************************
     * Platform Functions
     ******************************************************************* */

     function platformUpdateOrigin() {
         platform.update_bounds();
         let dev = settings.device;
         let proc = settings.process;
         let x = 0;
         let y = 0;
         let z = 0;
         if (MODE === MODES.CAM && proc.camOriginTop) {
             z = camTopZ + 0.01;
             if (!camStock) {
                 z += proc.camZTopOffset;
             }
         }
         if (!proc.outputOriginCenter) {
             if (camStock) {
                 x = (-camStock.scale.x / 2) + camStock.position.x;
                 y = (camStock.scale.y / 2) - camStock.position.y;
             } else {
                 if (MODE === MODES.LASER && proc.outputOriginBounds) {
                     let b = settings.bounds;
                     x = b.min.x,
                     y = -b.min.y
                 } else {
                     x = -dev.bedWidth / 2;
                     y = dev.bedDepth / 2;
                 }
             }
         } else if (camStock) {
             x = camStock.position.x;
             y = -camStock.position.y;
         }
         settings.origin = {x, y, z};
         if (settings.controller.showOrigin && MODE !== MODES.SLA) {
             SPACE.platform.setOrigin(x,y,z);
         } else {
             SPACE.platform.setOrigin();
         }
     }

     function platformUpdateTopZ() {
         let alignTopOk = WIDGETS.length > 1 && settings.controller.alignTop;
         let camz = (MODE === MODES.CAM) && (settings.stock.z || alignTopOk);
         let ztop = camz ? camTopZ - settings.process.camZTopOffset : 0;
         forAllWidgets(function(widget) {
             widget.setTopZ(ztop);
         });
     }

    function platformUpdateSize() {
        let frozen = SPACE.scene.freeze(true);
        let dev = settings.device,
            width, depth,
            height = Math.round(Math.max(dev.bedHeight, dev.bedWidth/100, dev.bedDepth/100));
        SPACE.platform.setRound(dev.bedRound);
        SPACE.platform.setSize(
            width = parseInt(dev.bedWidth),
            depth = parseInt(dev.bedDepth),
            height = parseFloat(dev.bedHeight)
        );
        SPACE.platform.setGZOff(height/2 - 0.1);
        platform.update_origin();
        SPACE.scene.freeze(frozen);
    }

    function platformUpdateBounds() {
        let bounds = new THREE.Box3();
        forAllWidgets(function(widget) {
            let wp = widget.track.pos;
            let wb = widget.mesh.getBoundingBox().clone();
            wb.min.x += wp.x;
            wb.max.x += wp.x;
            wb.min.y += wp.y;
            wb.max.y += wp.y;
            bounds.union(wb);
        });
        return settings.bounds = bounds;
    }

    function platformSelect(widget, shift) {
        if (viewMode !== VIEWS.ARRANGE) return;
        let mesh = widget.mesh,
            sel = (selectedMeshes.indexOf(mesh) >= 0);
        if (sel) {
            if (shift) {
                platform.deselect(widget)
            } else if (selectedMeshes.length > 1) {
                platform.deselect();
                platform.select(widget, false);
            }
        } else {
            // prevent selection in slice view
            if (!mesh.material.visible) return;
            if (!shift) platform.deselect();
            selectedMeshes.push(mesh);
            API.event.emit('widget.select', widget);
            widget.setColor(color.selected, settings);
            updateSelectedInfo();
        }
        platformUpdateSelected();
        SPACE.update();
    }

    function platformSelectedCount() {
        return viewMode === VIEWS.ARRANGE ? selectedMeshes.length : 0;
    }

    function platformUpdateSelected() {
        UI.selection.style.display = platform.selected_count() ? 'inline' : 'none';
        $('ext-sel').style.display = (MODE === MODES.FDM) ? 'inline-block' : 'none';
        let extruders = settings.device.extruders;
        if (extruders) {
            for (let i=0; i<extruders.length; i++) {
                let b = $(`sel-ext-${i}`);
                if (b) b.classList.remove('buton');
            }
            forSelectedWidgets(w => {
                let ext = settings.widget[w.id].extruder || 0;
                let b = $(`sel-ext-${ext}`);
                if (b) b.classList.add('buton');
            });
        }
    }

    function platformDeselect(widget) {
        if (viewMode !== VIEWS.ARRANGE) {
            // don't de-select and re-color widgets in,
            // for example, sliced or preview modes
            return;
        }
        if (!widget) {
            forAllWidgets(function(widget) {
                platform.deselect(widget);
            });
            return;
        }
        let mesh = widget.mesh,
            si = selectedMeshes.indexOf(mesh),
            sel = (si >= 0);
        if (sel) {
            selectedMeshes.splice(si,1);
            API.event.emit('widget.deselect', widget);
        }
        widget.setColor(color.deselected, settings);
        platformUpdateSelected();
        SPACE.update();
        updateSelectedInfo();
    }

    function platformLoad(url, onload) {
        console.log("It's me, platformLoad!");
        console.log(url);
        console.log(onload);
        if (url.toLowerCase().indexOf(".stl") > 0) {
            platform.load_stl(url, onload);
        } else {
            ajax(url, function(vertices) {
                vertices = js2o(vertices).toFloat32();
                platform.add(newWidget().loadVertices(vertices));
                if (onload) onload(vertices);
            });
        }
    }

    function platformLoadSTL(url, onload) {
        console.log("It's me, platformLoadSTL!");
        console.log(url);
        console.log(onload);
        new MOTO.STL().load(url, function(vertices) {
            platform.add(newWidget().loadVertices(vertices));
            if (onload) onload(vertices);
        })
    }

    function platformComputeMaxZ() {
        topZ = 0;
        forAllWidgets(function(widget) {
            topZ = Math.max(topZ, widget.mesh.getBoundingBox().max.z);
        });
        SPACE.platform.setMaxZ(topZ);
    }

    function platformGroup() {
        grouping = true;
    }

    // called after all new widgets are loaded to update group positions
    function platformGroupDone(skipLayout) {
        grouping = false;
        Widget.Groups.loadDone();
        if (layoutOnAdd && !skipLayout) platform.layout();
    }

    function platformAdd(widget, shift, nolayout) {
        if (!settings.widget[widget.id]) {
            settings.widget[widget.id] = {extruder: 0};
        }
        WIDGETS.push(widget);
        SPACE.platform.add(widget.mesh);
        platform.select(widget, shift);
        platform.compute_max_z();
        API.event.emit('widget.add', widget);
        if (nolayout) return;
        if (!grouping) {
            platformGroupDone();
        } else if (layoutOnAdd) {
            platform.layout();
        }
    }

    function platformDelete(widget) {
        if (!widget) {
            return;
        }
        if (Array.isArray(widget)) {
            let mc = widget.slice(), i;
            for (i=0; i<mc.length; i++) {
                platform.delete(mc[i].widget);
            }
            return;
        }
        KIRI.work.clear(widget);
        delete settings.widget[widget.id];
        WIDGETS.remove(widget);
        Widget.Groups.remove(widget);
        SPACE.platform.remove(widget.mesh);
        selectedMeshes.remove(widget.mesh);
        updateSliderMax();
        platform.compute_max_z();
        if (MODE !== MODES.FDM) platform.layout();
        SPACE.update();
        platformUpdateSelected();
        if (layoutOnAdd) platform.layout();
        API.event.emit('widget.delete', widget);
    }

    function platformSelectAll() {
        forAllWidgets(function(w) { platform.select(w, true) })
    }

    function platformLayout(event, space) {
        let auto = UI.autoLayout.checked,
            proc = settings.process,
            oldmode = viewMode,
            layout = (viewMode === VIEWS.ARRANGE && auto),
            topZ = MODE === MODES.CAM ? camTopZ - proc.camZTopOffset : 0;

        switch (MODE) {
            case MODES.SLA:
                space = space || (proc.slaSupportLayers && proc.slaSupportDensity ? 2 : 1);
                break;
            case MODES.CAM:
            case MODES.LASER:
                space = space || proc.outputTileSpacing || 1;
                break;
            case MODES.FDM:
                space = space || ((proc.sliceSupportExtra || 0) * 2) + 1;
                break;
        }

        setViewMode(VIEWS.ARRANGE);
        hideSlices();

        // only auto-layout when in arrange mode
        if (oldmode !== VIEWS.ARRANGE) {
            return SPACE.update();
        }

        // do not layout when switching back from slice view
        if (!auto || (!space && !layout)) {
            return SPACE.update();
        }

        let gap = space;

        // in CNC mode with >1 widget, force layout with spacing @ 1.5x largest tool diameter
        if (MODE === MODES.CAM && WIDGETS.length > 1) {
            let spacing = space || 1, CAM = KIRI.driver.CAM;
            if (proc.roughingOn) spacing = Math.max(spacing, CAM.getToolDiameter(settings, proc.roughingTool));
            if (proc.finishingOn || proc.finishingXOn || proc.finishingYOn) spacing = Math.max(spacing, CAM.getToolDiameter(settings, proc.finishingTool));
            gap = spacing * 1.5;
        }

        let i, m, sz = SPACE.platform.size(),
            mp = [sz.x, sz.y],
            ms = [mp[0] / 2, mp[1] / 2],
            mi = mp[0] > mp[1] ? [(mp[0] / mp[1]) * 10, 10] : [10, (mp[1] / mp[1]) * 10],
            c = Widget.Groups.blocks().sort(function (a, b) { return (b.w * b.h) - (a.w * a.h) }),
            p = new MOTO.Pack(ms[0], ms[1], gap).fit(c);

        while (!p.packed) {
            ms[0] += mi[0];
            ms[1] += mi[1];
            p = new MOTO.Pack(ms[0], ms[1], gap).fit(c);
        }
        console.log(c);
        for (i = 0; i < c.length; i++) {
            
            m = c[i];
            console.log(m);
            m.fit.x += m.w / 2 + p.pad;
            m.fit.y += m.h / 2 + p.pad;
            m.move(p.max.w / 2 - m.fit.x, p.max.h / 2 - m.fit.y, 0, true);
            // m.material.visible = true;
        }

        if (MODE === MODES.CAM) {
            platform.update_stock(true);
        }
        platform.update_origin();

        SPACE.update();
    }

    function platformUpdateStock(refresh) {
        let sd = settings.process;
        let offset = UI.camStockOffset.checked;
        let stockSet = offset || (sd.camStockX && sd.camStockY && sd.camStockZ > 0);
        let scale = unitScale();
        settings.stock = { };
        camTopZ = topZ;
        // create/inject cam stock if stock size other than default
        if (MODE === MODES.CAM && stockSet && WIDGETS.length) {
            UI.stock.style.display = offset ? 'inline-block' : 'none';
            let csx = sd.camStockX * scale;
            let csy = sd.camStockY * scale;
            let csz = sd.camStockZ * scale;
            let csox = 0;
            let csoy = 0;
            if (offset) {
                let min = { x: Infinity, y: Infinity, z: 0 };
                let max = { x: -Infinity, y: -Infinity, z: -Infinity };
                forAllWidgets(function(widget) {
                    let wbnd = widget.getBoundingBox(refresh);
                    let wpos = widget.track.pos;
                    min = {
                        x: Math.min(min.x, wpos.x + wbnd.min.x),
                        y: Math.min(min.y, wpos.y + wbnd.min.y),
                        z: 0
                    };
                    max = {
                        x: Math.max(max.x, wpos.x + wbnd.max.x),
                        y: Math.max(max.y, wpos.y + wbnd.max.y),
                        z: Math.max(max.z, wbnd.max.z)
                    };
                });
                csx += max.x - min.x;
                csy += max.y - min.y;
                csz += max.z - min.z;
                csox = min.x + ((max.x - min.x) / 2);
                csoy = min.y + ((max.y - min.y) / 2);
                $('stock-width').innerText = (csx/scale).toFixed(2);
                $('stock-depth').innerText = (csy/scale).toFixed(2);
                $('stock-height').innerText = (csz/scale).toFixed(2);
            }
            if (!camStock) {
                let geo = new THREE.BoxGeometry(1, 1, 1);
                let mat = new THREE.MeshBasicMaterial({ color: 0x777777, opacity: 0.2, transparent: true, side:THREE.DoubleSide });
                let cube = new THREE.Mesh(geo, mat);
                SPACE.platform.add(cube);
                camStock = cube;
            }
            settings.stock = {
                x: csx,
                y: csy,
                z: csz
            };
            camStock.scale.x = csx;
            camStock.scale.y = csy;
            camStock.scale.z = csz;
            camStock.position.x = csox;
            camStock.position.y = csoy;
            camStock.position.z = csz / 2;
            camStock.material.visible = settings.mode === 'CAM';
            camTopZ = csz;
        } else if (camStock) {
            UI.stock.style.display = 'none';
            SPACE.platform.remove(camStock);
            camStock = null;
            camTopZ = topZ;
        }
        platform.update_top_z();
        platform.update_origin();
        SPACE.update();
    }

    /** ******************************************************************
     * Settings Functions
     ******************************************************************* */

    // given a settings region, update values of matching bound UI fields
    function updateFieldsFromSettings(scope) {
        if (!scope) return console.trace("missing scope");
        for (let key in scope) {
            if (!scope.hasOwnProperty(key)) continue;
            let val = scope[key];
            if (UI.hasOwnProperty(key)) {
                let uie = UI[key], typ = uie ? uie.type : null;
                if (typ === 'text') {
                    uie.value = val;
                } else if (typ === 'checkbox') {
                    uie.checked = val;
                } else if (typ === 'select-one') {
                    uie.innerHTML = '';
                    let source = uie.parentNode.getAttribute('source'),
                        list = settings[source] || lists[source],
                        chosen = null;
                    if (list) list.forEach(function(tool, index) {
                        let id = tool.id || tool.name;
                        if (val == id) {
                            chosen = index;
                        }
                        let opt = DOC.createElement('option');
                        opt.appendChild(DOC.createTextNode(tool.name));
                        opt.setAttribute('value', id);
                        uie.appendChild(opt);
                    });
                    if (chosen) uie.selectedIndex = chosen;
                } else if (typ === 'textarea') {
                    if (Array.isArray(val)) {
                        uie.value = val.join('\n');
                    } else {
                        uie.value = '';
                    }
                }
            }
        }
    }

    /**
     * @returns {Object}
     */
    function updateSettingsFromFields(scope) {
        if (!scope) return console.trace("missing scope");

        let key, changed = false;

        // for each key in scope object
        for (key in scope) {
            if (!scope.hasOwnProperty(key)) {
                continue;
            }
            if (UI.hasOwnProperty(key)) {
                let nval = null, uie = UI[key];
                // skip empty UI values
                if (!uie || uie === '') {
                    continue;
                }
                if (uie.type === 'text') {
                    nval = UI[key].convert();
                } else if (uie.type === 'checkbox') {
                    nval = UI[key].checked;
                } else if (uie.type === 'select-one') {
                    if (uie.selectedIndex >= 0) {
                        nval = uie.options[uie.selectedIndex].value;
                        let src = uie.parentNode.getAttribute('source');
                        if (src === 'tools') {
                            nval = parseInt(nval);
                        }
                    } else {
                        nval = scope[key];
                    }
                } else if (uie.type === 'textarea') {
                    nval = uie.value.trim().split('\n').filter(v => v !== '');
                } else {
                    continue;
                }
                if (scope[key] != nval) {
                    scope[key] = nval;
                }
            }
        }

        return settings;
    }

    function updateFields() {
        updateFieldsFromSettings(settings.device);
        updateFieldsFromSettings(settings.process);
        updateFieldsFromSettings(settings.layers);
        updateFieldsFromSettings(settings.controller);
        updateExtruderFields(settings.device);
    }

    function updateExtruderFields(device) {
        if (device.extruders && device.extruders[device.internal]) {
            updateFieldsFromSettings(device.extruders[device.internal]);
            UI.extruder.firstChild.innerText = `${LANG.dv_gr_ext} [${device.internal+1}/${device.extruders.length}]`;
            UI.extPrev.disabled = device.internal === 0;
            UI.extPrev.onclick = function() {
                device.internal--;
                updateExtruderFields(device);
            };
            UI.extNext.disabled = device.internal === device.extruders.length - 1;
            UI.extNext.onclick = function() {
                device.internal++;
                updateExtruderFields(device);
            };
            UI.extDel.disabled = device.extruders.length < 2;
            UI.extDel.onclick = function() {
                device.extruders.splice(device.internal,1);
                device.internal = Math.min(device.internal, device.extruders.length-1);
                updateExtruderFields(device);
            };
            UI.extAdd.onclick = function() {
                let copy = clone(device.extruders[device.internal]);
                copy.extSelect = [`T${device.extruders.length}`];
                device.extruders.push(copy);
                device.internal = device.extruders.length - 1;
                updateExtruderFields(device);
            };
        }
    }

    function updateSettings() {
        updateSettingsFromFields(settings.controller);
        updateSettingsFromFields(settings.device);
        updateSettingsFromFields(settings.process);
        updateSettingsFromFields(settings.layers);
        let device = settings.device;
        if (device.extruders && device.extruders[device.internal]) {
            updateSettingsFromFields(device.extruders[device.internal]);
        }
        API.conf.save();
        platform.update_stock();
    }

    function saveSettings() {
        let view = SPACE.view.save();
        if (view.left || view.up) {
            settings.controller.view = view;
        }
        SDB.setItem('ws-settings', JSON.stringify(settings));
    }

    function settingsImport(data, ask) {
        if (typeof(data) === 'string') {
            try {
                data = JSON.parse(atob(data));
            } catch (e) {
                alert('invalid settings format');
                console.log('data',data);
                return;
            }
        }
        if (!(data.settings && data.version && data.time)) {
            alert('invalid settings format');
            console.log('data',data);
            return;
        }
        if (ask && !confirm(`Import settings made on ${new Date(data.time)} from Kiri:Moto version ${data.version}?`)) {
            return;
        }
        settings = CONF.normalize(data.settings);
        API.conf.save();
        API.space.reload();
    }

    function settingsExport() {
        return btoa(JSON.stringify({
            settings: settings,
            version: KIRI.version,
            moto: MOTO.id,
            init: SDB.getItem('kiri-init'),
            time: Date.now()
        }));
    }

    function platformLoadFiles(files,group,isScrap) {
        // console.log("PlatformLoad", isScrap);
        let loaded = files.length;
        if (!isScrap) {
            platform.group();
        }
        for (let i=0; i<files.length; i++) {
            let reader = new FileReader(),
                lower = files[i].name.toLowerCase(),
                israw = lower.indexOf(".raw") > 0 || lower.indexOf('.') < 0,
                isstl = lower.indexOf(".stl") > 0,
                issvg = lower.indexOf(".svg") > 0,
                isgcode = lower.indexOf(".gcode") > 0 || lower.indexOf(".nc") > 0,
                isset = lower.indexOf(".b64") > 0;
            reader.file = files[i];
            reader.onloadend = function (e) {
                if (israw) platform.add(
                    newWidget(undefined,group)
                    .loadVertices(JSON.parse(e.target.result).toFloat32())
                );
                if (isstl) {
                    // console.log("It's me, platformLoadFiles!");
                    // console.log(files);
                    // console.log(group);
                    // console.log(API.feature.on_add_stl);
                    if (API.feature.on_add_stl) {
                        API.feature.on_add_stl(e.target.result);
                    } 
                    else if (isScrap) {
                        
                        let vertices = new MOTO.STL().parse(e.target.result);
                        let geometry = new THREE.BufferGeometry();
                        geometry.setAttribute('position', new THREE.BufferAttribute(vertices, 3));

                        let mesh = new THREE.Mesh(
                            geometry,
                            new THREE.MeshPhongMaterial({
                                color: 0xffff00,
                                specular: 0x181818,
                                shininess: 100,
                                transparent: true,
                                opacity: 1.0
                            })
                        );
                        // fix invalid normals
                        geometry.computeFaceNormals();
                        geometry.computeVertexNormals();
                        // to fix mirroring of normals not working as expected
                        mesh.material.side = THREE.DoubleSide;
                        mesh.castShadow = true;
                        mesh.receiveShadow = true;
                        // mesh.widget = this;
                        // this.mesh = mesh;
                        // invalidates points cache (like any scale/rotation)
                        // this.center(true);
                        // LWW TODO: Move mesh by parent move (check widget PRO center)

                        SCRAPS.push(mesh);
                        SCRAPS.push({}); // fill object later during prepareSlices
                        SCRAPS.push(group);
                        SCRAPS.push(SCRAPSMOVE[0]); // translation
                        // LWW TODO: Add a number which model (modelmove/SCRAPSMOVE) this belongs to, make slicing only use those selected scraps
                        // console.log(SCRAPS);

                        // let extractedData = slicer.prepareScrap(SCRAPS, Widget);
                        // SCRAPS[SCRAPS.length-3] = extractedData;
                        // console.log(SCRAPS);




                        //var scrapWidget = newWidget(undefined,group).loadVertices(new MOTO.STL().parse(e.target.result));
                        //scrapWidget.isScrap = true;
                        //console.log(scrapWidget);
                        //SCRAPS.push(scrapWidget);
                    }
                    
                    else {
                        var modelWidget = newWidget(undefined,group)
                        .loadVertices(new MOTO.STL().parse(e.target.result))
                        .saveToCatalog(e.target.file.name);
                        // console.log(modelWidget);
                        platform.add(
                            // newWidget(undefined,group)
                            // .loadVertices(new MOTO.STL().parse(e.target.result))
                            // .saveToCatalog(e.target.file.name)
                            modelWidget
                        );
                        console.log("Loaded Widget", modelWidget);
                        console.log("Loaded Widget BB", modelWidget.getBoundingBox());

                        // LWW TODO: Also keep track fo platform position for multiple objects to slice

                        // LWW TODO: Save position from other load function (+) as well

                        let bb = modelWidget.getBoundingBox(true),
                            bm = bb.min.clone(),
                            bM = bb.max.clone(),
                            bd = bM.sub(bm).multiplyScalar(0.5),
                            dx = bm.x + bd.x,
                            dy = bm.y + bd.y,
                            dz = bm.z;

                        SCRAPSMOVE = [{x: dx, y: dy, z: dz}];
                    }
                }
                if (isgcode) loadCode(e.target.result, 'gcode');
                if (issvg) loadCode(e.target.result, 'svg');
                if (isset) settingsImport(e.target.result, true);
                if (--loaded === 0 && !isScrap) platform.group_done(isgcode);
            };
            reader.readAsBinaryString(reader.file);
        }
    }

    function loadFile(isScrap) {
        $('load-file').onchange = function(event) {
            DBUG.log(event);
            // console.log(event);
            platformLoadFiles(event.target.files, undefined, isScrap);
            // console.log(event.target.files);
            // console.log(event.target.files[0]);
            // var localFile = new File([""], "/obj/EmptyCube.stl", {type: "stl"});
            // console.log(localFile);
            //var localFileList = new FileList()
            //var localFileList = {0:localFile, length:1};
            //platformLoadFiles([localFile]);
            //platformLoadFiles(event.target.files);

            // Used for creating a new FileList in a round-about way
            // function FileListItem(a) {
            //     a = [].slice.call(Array.isArray(a) ? a : arguments)
            //     for (var c, b = c = a.length, d = !0; b-- && d;) d = a[b] instanceof File
            //     if (!d) throw new TypeError("expected argument to FileList is File or array of File objects")
            //     for (b = (new ClipboardEvent("")).clipboardData || new DataTransfer; c--;) b.items.add(a[c])
            //     return b.files
            // }
            
            // var files = [
            //     new File(['content'], '/obj/EmptyCube.stl', {type: "stl"}),
            // ];
            
            // //fileInput.files = new FileListItem(files)

            // var localFileList = new FileListItem(files);
            // platformLoadFiles(localFileList);

            // var localFile2 = '';
            // var xmlhttp = new XMLHttpRequest();
            // xmlhttp.onreadystatechange = function(){
            //     if(xmlhttp.status == 200 && xmlhttp.readyState == 4){
            //         localFile2 = xmlhttp.responseText;
            //         console.log("Async");
            //         console.log(localFile2);
            //         console.log("Async");
            //     }
            // };
            // xmlhttp.open("GET","/obj/EmptyCube.stl",true);
            // xmlhttp.send();
            // console.log("localFile2");
            // console.log(localFile2);
            // console.log("localFile2");
        };
        $('load-file').click();
        // alert2("drag/drop STL files onto platform to import\nreload page to return to last saved state");
    }

    function saveWorkspace() {
        API.conf.save();
        let newWidgets = [],
            oldWidgets = js2o(SDB.getItem('ws-widgets'), []);
        forAllWidgets(function(widget) {
            newWidgets.push(widget.id);
            oldWidgets.remove(widget.id);
            widget.saveState();
        });
        SDB.setItem('ws-widgets', o2js(newWidgets));
        oldWidgets.forEach(function(wid) {
            Widget.deleteFromState(wid);
        });
        alert2("workspace saved", 1);
    }

    function restoreSettings(save) {
        let newset = ls2o('ws-settings') || settings;

        settings = CONF.normalize(newset);
        // override camera from settings
        if (settings.controller.view) {
            SDB.removeItem('ws-camera');
        }
        // merge custom filters from localstorage into settings
        localFilters.forEach(function(fname) {
            let fkey = "gcode-filter-"+fname, ov = ls2o(fkey);
            if (ov) settings.devices[fname] = ov;
            SDB.removeItem(fkey)
        });
        SDB.removeItem(localFilterKey);
        // save updated settings
        if (save) API.conf.save();

        return newset;
    }

    function restoreWorkspace(ondone, skip_widget_load) {
        let newset = restoreSettings(true),
            camera = newset.controller.view,
            toload = ls2o('ws-widgets',[]),
            loaded = 0,
            position = true;

        updateFields();
        platform.update_size();
        platform.update_stock();

        let fz = SPACE.scene.freeze(true);
        SPACE.view.reset();
        if (camera) {
            SPACE.view.load(camera);
        } else {
            SPACE.view.home();
        }
        SPACE.scene.freeze(fz);

        if (skip_widget_load) return;

        // remove any widgets from platform
        forAllWidgets(function(widget) {
            platform.delete(widget);
        });

        // load any widget by name that was saved to the workspace
        toload.forEach(function(widgetid) {
            Widget.loadFromState(widgetid, function(widget) {
                if (widget) {
                    platform.add(widget, 0, position);
                }
                if (++loaded === toload.length) {
                    platform.deselect();
                    if (ondone) {
                        ondone();
                        setTimeout(() => {
                            platform.update_top_z();
                            SPACE.update();
                        }, 1);
                    }
                }
            }, position);
        });

        return toload.length > 0;
    }

    function clearWorkspace() {
        // free up worker cache/mem
        KIRI.work.clear();
        platform.select_all();
        platform.delete(selectedMeshes);
    }

    function modalShowing() {
        let showing = $('modal').style.display !== 'none';
        return showing || UC.isPopped();
    }

    function showModal(which) {
        UI.modal.style.display = 'block';
        ["print","help","local"].forEach(function(modal) {
            UI[modal].style.display = (modal === which ? 'block' : 'none');
        });
        if (which) API.event.emit('modal.show', which);
    }

    function hideDialog() {
        showDialog(null);
    }

    function showDialog(which, force) {
        if (UC.isPopped()) {
            UC.hidePop();
            return;
        }
        ["catalog","devices","tools","settings"].forEach(function(dialog) {
            let style = UI[dialog].style;
            style.display = (dialog === which && (force || style.display !== 'flex') ? 'flex' : 'none');
        });
        if (which) API.event.emit('dialog.show', which);
    }

    function showCatalog() {
        showDialog("catalog");
    }

    function getSettings() {
        return settings;
    }

    function putSettings(newset) {
        settings = CONF.normalize(newset);
        API.conf.save()
        API.space.restore(null, true);
    }

    function editSettings(e) {
        let mode = getMode(),
            name = e.target.getAttribute("name"),
            load = settings.sproc[mode][name],
            edit = prompt(`settings for "${name}"`, JSON.stringify(load));

        if (edit) {
            try {
                settings.sproc[mode][name] = JSON.parse(edit);
                if (name === settings.process.processName) {
                    API.conf.load(null, name);
                }
                API.conf.save();
            } catch (e) {
                alert('malformed settings object');
            }
        }
    }

    function loadSettings(e, named) {
        let mode = getMode(),
            name = e ? e.target.getAttribute("load") : named || settings.cproc[mode],
            load = settings.sproc[mode][name];

        if (!load) return;

        settings.process = clone(load);
        settings.process.processName = name;
        settings.cproc[mode] = name;

        // associate named process with the current device
        settings.devproc[currentDeviceName()] = name;

        // update selection display
        $('selected-mode').innerHTML = API.mode.get();
        $('selected-device').innerHTML = API.device.get();
        $('selected-process').innerHTML = name;

        // FDM process settings overridden by device
        if (mode == "FDM") {
            settings.process.outputOriginCenter = (settings.device.originCenter || false);
        }

        if (!named) {
            hideDialog();
        }

        updateFields();
        API.conf.update();
        if (e) triggerSettingsEvent();
    }

    function deleteSettings(e) {
        let name = e.target.getAttribute("del");
        delete settings.sproc[getMode()][name];
        updateSettingsList();
        API.conf.save();
        triggerSettingsEvent();
    }

    function updateSettingsList() {
        let list = [], s = settings, sp = s.sproc[getMode()] || {}, table = UI.settingsList;
        table.innerHTML = '';
        for (let k in sp) {
            if (sp.hasOwnProperty(k)) list.push(k);
        }
        list.sort().forEach(function(sk) {
            let row = DOC.createElement('div'),
                load = DOC.createElement('button'),
                edit = DOC.createElement('button'),
                del = DOC.createElement('button'),
                name = sk;

            load.setAttribute('load', sk);
            load.onclick = API.conf.load;
            load.appendChild(DOC.createTextNode(sk));
            if (sk == settings.process.processName) {
                load.setAttribute('class', 'selected')
            }

            del.setAttribute('del', sk);
            del.setAttribute('title', "remove '"+sk+"'");
            del.onclick = deleteSettings;
            del.appendChild(DOC.createTextNode('x'));

            edit.innerHTML = '&uarr;';
            edit.setAttribute('name', sk);
            edit.setAttribute('title', 'edit');
            edit.onclick = editSettings;

            row.setAttribute("class", "flow-row");
            row.appendChild(edit);
            row.appendChild(load);
            row.appendChild(del);
            table.appendChild(row);
        });
        API.dialog.update();
    }

    function showSettings() {
        updateSettingsList();
        showDialog("settings");
    }

    function updateDialogLeft() {
        let left = UI.ctrlLeft.getBoundingClientRect();
        let right = UI.ctrlRight.getBoundingClientRect();
        UI.catalog.style.left = (left.width + 5) + 'px';
        UI.devices.style.left = (left.width + 5) + 'px';
        UI.tools.style.left = (left.width + 5) + 'px';
        UI.settings.style.right = (right.width + 5) + 'px';
    }

    function hideModal() {
        UI.modal.style.display = 'none';
    }

    function showHelp() {
        showHelpFile(`/kiri/lang/${KIRI.lang.get()}-help.html`);
    }

    function showHelpFile(local) {
        hideDialog();
        if (!local) {
            WIN.open("//wiki.grid.space/wiki/Kiri:Moto", "_help");
            return;
        }
        ajax(local, function(html) {
            UI.help.innerHTML = html;
            $('help-close').onclick = hideModal;
            $('kiri-version').innerHTML = `<i>${LANG.version} ${KIRI.version}</i>`;
            showModal('help');
        });
        API.event.emit('help.show', local);
    }

    function showLocal() {
        $('local-close').onclick = hideModal;
        showModal('local');
        fetch("/api/grid_local")
            .then(r => r.json())
            .then(j => {
                let bind = [];
                let html = ['<table>'];
                html.push(`<thead><tr><th>device</th><th>type</th><th>status</th><th></th></tr></thead>`);
                html.push(`<tbody>`);
                for (let k in j) {
                    let r = j[k].stat;
                    bind.push({uuid: r.device.uuid, host: r.device.addr[0], post: r.device.port});
                    html.push(`<tr>`);
                    html.push(`<td>${r.device.name}</td>`);
                    html.push(`<td>${r.device.mode}</td>`);
                    html.push(`<td>${r.state}</td>`);
                    html.push(`<td><button id="${r.device.uuid}">admin</button></td>`);
                    html.push(`</tr>`);
                }
                html.push(`</tbody>`);
                html.push(`</table>`);
                $('local-dev').innerHTML = html.join('');
                bind.forEach(rec => {
                    $(rec.uuid).onclick = () => {
                        window.open(`http://${rec.host}:${rec.port||4080}/`);
                    };
                });
            });
    }

    function setFocus(el) {
        el = [ el || UI.load, UI.import, UI.ctrlLeft, UI.container, UI.assets, UI.control, UI.modeFDM, UI.reverseZoom, UI.modelOpacity, DOC.body ];
        for (let es, i=0; i<el.length; i++) {
            es = el[i];
            es.focus();
            if (DOC.activeElement === es) {
                break;
            }
        }
    }

    function setViewMode(mode) {
        let oldMode = viewMode;
        viewMode = mode;
        platform.deselect();
        updateSelectedInfo();
        switch (mode) {
            case VIEWS.ARRANGE:
                KIRI.work.clear();
                clearWidgetCache();
                showLayerView(false);
                updateSliderMax();
                showModeActive(UI.modeArrange);
                break;
            case VIEWS.SLICE:
                showLayerView(true);
                updateSliderMax();
                showModeActive(UI.modeSlice);
                break;
            case VIEWS.PREVIEW:
                showLayerView(true);
                showModeActive(UI.modePreview);
                break;
            default:
                DBUG.log("invalid view mode: "+mode);
                return;
        }
        DOC.activeElement.blur();
    }

    function showModeActive(el) {
        [ UI.modeArrange, UI.modeSlice, UI.modePreview, UI.modeExport ].forEach(function(b) {
            if (b === el) {
                b.classList.add('buton');
            } else {
                b.classList.remove('buton');
            }
        });
    }

    function showLayerView(bool) {
        UI.layerView.style.display = bool ? 'flex' : 'none';
    }

    function setExpert(bool) {
        UC.setExpert(UI.expert.checked = control.expert = bool);
    }

    function getMode() {
        return settings.mode;
    }

    function getModeLower() {
        return getMode().toLowerCase();
    }

    function switchMode(mode) {
        setMode(mode, platform.update_size);
    }

    function setMode(mode, lock, then) {
        hideModal();
        hideDialog();
        if (!MODES[mode]) {
            DBUG.log("invalid mode: "+mode);
            mode = 'FDM';
        }
        settings.mode = mode;
        // restore cached device profile for this mode
        if (settings.cdev[mode]) {
            settings.device = clone(settings.cdev[mode]);
            API.event.emit('device.set', currentDeviceName());
        }
        if (MODE === MODES.CAM) {
            SPACE.platform.setColor(0xeeeeee);
        } else {
            SPACE.platform.setColor(0xcccccc);
        }
        MODE = MODES[mode];
        // updates right-hand menu by enabling/disabling fields
        setViewMode(VIEWS.ARRANGE);
        UC.setMode(MODE);
        API.conf.load();
        API.conf.save();
        clearWidgetCache();
        SPACE.update();
        UI.modeFDM.setAttribute('class', MODE === MODES.FDM ? 'buton' : '');
        UI.modeSLA.setAttribute('class', MODE === MODES.SLA ? 'buton' : '');
        UI.modeCAM.setAttribute('class', MODE === MODES.CAM ? 'buton' : '');
        UI.modeLASER.setAttribute('class', MODE === MODES.LASER ? 'buton' : '');
        UI.mode.style.display = lock ? 'none' : '';
        UI.modeTable.style.display = lock ? 'none' : '';
        UI.modelOpacity.style.display = MODE === MODES.SLA ? 'none' : '';
        UI.modePreview.style.display = MODE === MODES.SLA ? 'none' : '';
        if (camStock) {
            camStock.material.visible = settings.mode === 'CAM';
        }
        API.space.restore(null, true);
        if (then) then();
        triggerSettingsEvent();
        platformUpdateSelected();
        updateFields();
        if (settings.device.new) {
            API.show.devices();
        }
        API.event.emit("mode.set", mode);
    }

    function currentDeviceName() {
        return settings.filter[getMode()];
    }

    function setControlsVisible(show) {
        UI.ctrlLeft.style.display = show ? 'block' : 'none';
        UI.ctrlRight.style.display = show ? 'block' : 'none';
    }

    // prevent safari from exiting full screen mode
    DOC.onkeydown = function (evt) { if (evt.keyCode == 27) evt.preventDefault() }

    // run optional module functions NOW before kiri-init has run
    if (Array.isArray(self.kirimod)) {
        kirimod.forEach(function(mod) { mod(kiri.api) });
    }

})();
