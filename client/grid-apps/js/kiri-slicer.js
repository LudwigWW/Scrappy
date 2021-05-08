/** Copyright 2014-2019 Stewart Allen -- All Rights Reserved */

"use strict";

let gs_kiri_slicer = exports;

(function() {

    if (!self.kiri) self.kiri = {};
    if (self.kiri.slicer) return;

    let slicer = self.kiri.slicer = {
        slice: slice,
        sliceWidget: sliceWidget,
        connectLines: connectLines,
        prepareScrap: prepareScrap
    };

    let KIRI = self.kiri,
        BASE = self.base,
        CONF = BASE.config,
        UTIL = BASE.util,
        POLY = BASE.polygons,
        time = UTIL.time,
        newSlice = KIRI.newSlice,
        newOrderedLine = BASE.newOrderedLine;
    
    /**
     * Convenience method. Gets a Widget's points and calls slice()
     *
     * @param {Widget} widget
     * @param {Object} options
     * @param {Function} ondone callback when slicing complete
     * @param {Function} onupdate callback on incremental updates
     */
    function sliceWidget(widget, options, ondone, onupdate, ondoneScrap, onupdateScrap, isScrap) {
        // console.log("kiri-slicer_sliceWidget");
        // console.log("sliceWidget", widget);
        slice(widget.getPoints(), widget.getBoundingBox(), options, ondone, onupdate, ondoneScrap, onupdateScrap, isScrap, widget.getScrapData());
    }


    /**
     * Slice saved scrap meshes
     * 
     * @param {Array} ScrapDataArray // Mesh, Groupdata, Translationdata, repeat...
     */
    function prepareScrap(ScrapDataArray, widgetPointer) {
        // console.log("kiri-slicer_sliceWidget");
        // console.log("Slicer_prepareScrap", KIRI.scraps);
        for (let scrapIndex = 0; scrapIndex < KIRI.scraps.length; scrapIndex = scrapIndex + 4) {
            
            let translation = KIRI.scraps[scrapIndex + 3];

            let copyMesh = clone(KIRI.scraps[scrapIndex]);
            let scrapMesh = KIRI.scraps[scrapIndex];
            // let gap = scrapMesh.geometry.attributes.position,
            //     pa = gap.array;

            // for (let meshIndex = 0; meshIndex < pa.length; meshIndex += 3) {
            //     console.log(translation);
            //     pa[meshIndex    ] -= translation.x;
            //     pa[meshIndex + 1] -= translation.y;
            //     pa[meshIndex + 2] -= translation.z;
            // }
            // gap.needsUpdate = true;

            let meshPointsObj = clone(scrapMesh.geometry.attributes.position.array);
            let meshPointsArray = Object.values(meshPointsObj);
            for (let meshIndex = 0; meshIndex < meshPointsArray.length; meshIndex += 3) {
                meshPointsArray[meshIndex    ] -= translation.x;
                meshPointsArray[meshIndex + 1] -= translation.y;
                meshPointsArray[meshIndex + 2] -= translation.z;
            }

            // console.log("copyMesh", copyMesh);
            // console.log("scrapMesh", scrapMesh);
            // var scrapPoints = KIRI.Widget.verticesToPoints(scrapMesh.geometry.getAttribute('position').array, true); // decimating = true I guess
            // console.log("meshPointsArray", meshPointsArray);
            // console.log("scrapMesh.geometry.attributes.position.array", scrapMesh.geometry.attributes.position.array);
            var scrapPoints = KIRI.Widget.verticesToPoints(meshPointsArray, true); // decimating = true I guess
            // console.log("scrapPoints", scrapPoints);
            var scrapBB = new THREE.Box3();
            scrapBB.setFromPoints(scrapPoints);
            // KIRI.scraps[scrapIndex + 1] = {points: scrapPoints, bounds: scrapBB};
            // LWW TODO: Make this not return after the first
            return {points: scrapPoints, bounds: scrapBB};
        }
    }


    function sliceScrap(points, bounds, options, ondone, onupdate, isScrap) {
        let topoMode = options.topo,
            simpleMode = options.simple,
            swap = options.swapX || options.swapY,
            isCam = options.cam,
            ox = 0,
            oy = 0;

        // handle rotating meshes for CAM finishing.
        // slicer expects things just so, so we alter
        // geometry to satisfy
        if (swap) {
            points = points.slice();

            let btmp = new THREE.Box3(),
                pref = {},
                cached;

            btmp.setFromPoints(points);
            if (options.swapX) ox = -btmp.max.x;
            if (options.swapY) oy = -btmp.max.y;

            // array re-uses points so we need
            // to be careful not to alter a point
            // more than once
            for (let p, index=0; index<points.length; index++) {
                p = points[index];
                cached = pref[p.key];
                // skip points already altered
                if (cached) {
                    points[index] = cached;
                    continue;
                }
                cached = p.clone();
                if (options.swapX) cached.swapXZ();
                else if (options.swapY) cached.swapYZ();
                cached.rekey();
                pref[p.key] = cached;
                points[index] = cached;
            }

            // update temp bounds from new points
            btmp.setFromPoints(points);
            for (let p, index=0; index<points.length; index++) {
                p = points[index];
                if (p.mod === 1) continue;
                p.mod = 1;
                p.z -= btmp.min.z;
            }

            // update temp bounds from points with altered Z
            btmp.setFromPoints(points);
            bounds = btmp;
        }

        let zMin = options.zmin || Math.floor(bounds.min.z),
            zMax = options.zmax || Math.ceil(bounds.max.z),
            zInc = options.height,
            zIncMin = options.minHeight,
            zIncFirst = options.firstHeight || zInc,
            zOff = true ? zInc / 2 : 0,
            zHeights = [],      // heights for zIndexes in adaptive mode
            zIndexes = [],      // auto-detected z slicing offsets (laser/cam)
            zOrdered = [],      // ordered list of Z indexes
            zList = {},         // map count of z index points for adaptive slicing
            zFlat = {},         // map area of z index flat areas (cam)
            zLines = {},        // map count of z index lines
            zScale,             // bucket span in z units
            timeStart = time(),
            slices = [],
            zSum = 0.0,
            buckets = [],
            i, j = 0, k, p1, p2, p3, px,
            CPRO = KIRI.driver.CAM.process;

        if (options.add) zMax += zInc;

        function countZ(z) {
            z = UTIL.round(z,5);
            zList[z] = (zList[z] || 0) + 1;
        }

        // gather z-index stats
        // these are used for auto-slicing in laser
        // and to flats detection in CAM mode
        for (i = 0; i < points.length;) {
            p1 = points[i++];
            p2 = points[i++];
            p3 = points[i++];
            zSum += (Math.abs(p1.z - p2.z) + Math.abs(p2.z - p3.z) + Math.abs(p3.z - p1.z));
            // laser auto-detect z slice points
            if (zInc === 0 || zIncMin) {
                countZ(p1.z);
                countZ(p2.z);
                countZ(p3.z);
            }
            // use co-flat and co-line detection to adjust slice Z
            if (p1.z === p2.z && p2.z === p3.z && p1.z > bounds.min.z) {
                // detect zFlat faces to avoid slicing directly on them
                let zkey = p1.z.toFixed(5),
                    area = Math.abs(UTIL.area2(p1,p2,p3)) / 2;
                if (!zFlat[zkey]) {
                    zFlat[zkey] = area;
                } else {
                    zFlat[zkey] += area;
                }
            } else if (true || options.trace) {
                // detect zLines (curved region tops/bottoms)
                // mark these layers for ball and v mill tracing
                if (p1.z === p2.z && p1.z > bounds.min.z) {
                    let zkey = p1.z.toFixed(5);
                    let zval = zLines[zkey];
                    zLines[zkey] = (zval || 0) + 1;
                }
                if (p2.z === p3.z && p2.z > bounds.min.z) {
                    let zkey = p2.z.toFixed(5);
                    let zval = zLines[zkey];
                    zLines[zkey] = (zval || 0) + 1;
                }
                if (p3.z === p1.z && p3.z > bounds.min.z) {
                    let zkey = p3.z.toFixed(5);
                    let zval = zLines[zkey];
                    zLines[zkey] = (zval || 0) + 1;
                }
            }
        }

        // CAM: find slice candidates to trace for ballmills and tapermills
        // if (options.trace) {
        //     let zl = {};
        //     let le;
        //     let zs = Object.entries(zLines).map(oe => {
        //         return oe.map(v => parseFloat(v));
        //     }).sort((a,b) => {
        //         return b[0] - a[0];
        //     }).forEach((e,i) => {
        //         if (i > 0) {
        //             let zd = le[0]-e[0];
        //             if (zd > 0.1 && e[1] > 100) {
        //                 // zl.push(e)
        //                 zl[e[0].toFixed(5)] = e[1];
        //                 zFlat[e[0].toFixed(5)] = e[1];
        //             }
        //         }
        //         if (e[1] > 10) {
        //             le = e;
        //         }
        //     });
        //     zLines = zl;
        // }

        /**
         * bucket polygons into z-bounded groups (inside or crossing)
         * to reduce the search space in complex models
         */
        let bucketCount = Math.max(1, Math.ceil(zMax / (zSum / points.length)) - 1);

        zScale = 1 / (zMax / bucketCount);

        if (bucketCount > 1) {
            // create empty buckets
            for (i = 0; i < bucketCount + 1; i++) buckets.push([]);

            // copy triples into all matching z-buckets
            for (i = 0; i < points.length;) {
                p1 = points[i++];
                p2 = points[i++];
                p3 = points[i++];
                let zm = Math.min(p1.z, p2.z, p3.z),
                    zM = Math.max(p1.z, p2.z, p3.z),
                    bm = Math.floor(zm * zScale),
                    bM = Math.ceil(zM * zScale);
                for (j = bm; j < bM; j++) {
                    buckets[j].push(p1);
                    buckets[j].push(p2);
                    buckets[j].push(p3);
                }
            }
        }

        // we need Z ordered list for laser auto or adaptive fdm slicing
        if (zInc === 0 || zIncMin) {
            for (let key in zList) {
                if (!zList.hasOwnProperty(key)) continue;
                zOrdered.push(parseFloat(key));
            }
            zOrdered.sort(function(a,b) { return a - b});
        }

        if (options.single) {
            // usually for laser single slice
            zIndexes.push(zMin + zInc);
        } else if (zInc === 0) {
            // use Z indices in auto slice mode for laser
            // find unique z-index offsets for slicing
            let zl = zOrdered
            for (i = 0; i < zl.length - 1; i++) {
                zIndexes.push((zl[i] + zl[i+1]) / 2);
            }
        } else if (isCam) {
            // re-divide slice height so that top and
            // bottom slices fall exactly on those faces
            zInc = (zMax - zMin) / (Math.floor(zMax / zInc) + 1);
            for (i = zMin; i < zMax; i += zInc) {
                zIndexes.push(i);
            }
            for (let key in zFlat) {
                // todo make threshold for flat detection configurable
                if (!zFlat.hasOwnProperty(key) || zFlat[key] < 10){
                    continue;
                }
                key = parseFloat(key);
                if (!zIndexes.contains(key) && key >= zMin) {
                    zIndexes.push(key);
                }
            }
            // sort top down
            zIndexes.sort(function(a,b) {
                return b-a;
            });
            // copy to store original z
            zHeights = zIndexes.slice();
        } else if (zIncMin) {
            // FDM adaptive slicing
            let zPos = zMin + zIncFirst,
                zOI = 0,
                zDelta,
                zDivMin,
                zDivMax,
                zStep,
                nextZ,
                lzp = zPos;

            // first slice is fixed
            zHeights.push(zIncFirst);
            zIndexes.push(zIncFirst / 2);

            while (zPos < zMax && zOI < zOrdered.length) {
                nextZ = zOrdered[zOI++];
                if (zPos >= nextZ) continue;
                zDelta = nextZ - zPos;
                if (zDelta < zIncMin) continue;

                zDivMin = Math.floor(zDelta / zIncMin);
                zDivMax = Math.floor(zDelta / zInc);

                if (zDivMax && zDivMax <= zDivMin) {
                    if (zDelta % zInc > 0.01) zDivMax++;
                    zStep = zDelta / zDivMax;
                    // console.log(`--- zDivMax <= zDivMin ---`, zStep, zDelta % zInc)
                } else {
                    zStep = zDelta;
                }
                // console.log({nextZ, zPos, zDelta, zStep, zDivMin, zDivMax})
                while (zPos < nextZ) {
                    zHeights.push(zStep);
                    zIndexes.push(zPos + zStep / 2);
                    zPos += zStep;
                    // console.log({D: zPos - lzp, zPos})
                    // lzp = zPos;
                }
            }
        } else {
            // FDM fixed slicing
            if (options.firstHeight) {
                zIndexes.push(options.firstHeight / 2);
                zHeights.push(options.firstHeight);
                zMin = options.firstHeight; // LWW TODO this maybe should be removed
            }
            for (i = zMin + zOff; i < zMax; i += zInc) {
                zIndexes.push(i);
                zHeights.push(zInc);
            }
        }
        // create a Slice for each z offset in the zIndexes array
        for (let i = 0; i < zIndexes.length; i++) {
            let ik = zIndexes[i].toFixed(5),
                onFlat = false,
                onLine = false;
            // ensure no slice through horizontal lines or planes
            if (zFlat[ik]) onFlat = true;
            if (zLines[ik]) onLine = true;
            if (onFlat || onLine) {
                zIndexes[i] += isCam ? 0.001 : -0.001;
            }
            // slice next layer and add to slices[] array
            let slice = sliceZScrap(zIndexes[i], zHeights[i], onFlat, onLine, isScrap);
            // override z in cam in case it was moved for a flat or line
            if (isCam && slice) {
                slice.z = zHeights[i];
            }
            onupdate(i / zIndexes.length);
        }

        // for cam, bottom as mandatory (hasFlats)
        if (isCam && slices.length > 0) {
            // slices[0].hasFlats = true;
            slices[slices.length-1].hasFlats = true;
        }

        // connect slices into linked list for island/bridge projections
        for (i=1; i<slices.length; i++) {
            slices[i-1].up = slices[i];
            slices[i].down = slices[i-1];
        }

        slices.slice_time = time() - timeStart;

        // pass Slices array back to ondone function
        ondone(slices);

        return slices;

        /** ***** SLICING FUNCTIONS ***** */

        /**
         * given a point, append to the correct
         * 'where' objec tarray (on, over or under)
         *
         * @param {Point} p
         * @param {number} z offset
         * @param {Obejct} where
         */
        function checkUnderOverOn(p, z, where) {
            let delta = p.z - z;
            if (Math.abs(delta) < CONF.precision_slice_z) { // on
                where.on.push(p);
            } else if (delta < 0) { // under
                where.under.push(p);
            } else { // over
                where.over.push(p);
            }
        }

        /**
         * for point, return z-plane intersecting point on line to next point
         *
         * @param {Point} p
         * @param {number} z
         * @returns {Point}
         */
        function intersectZ(startP, p, z) {
            var dx = p.x - startP.x,
                dy = p.y - startP.y,
                dz = p.z - startP.z,
                pct = 1 - ((p.z - z) / dz);
            return BASE.newPoint(startP.x + dx * pct, startP.y + dy * pct, startP.z + dz * pct);
        };

        /**
         * Given a point over and under a z offset, calculate
         * and return the intersection point on that z plane
         *
         * @param {Point} over
         * @param {Point} under
         * @param {number} z offset
         * @returns {Point} intersection point
         */
        function intersectPoints(over, under, z) {
            let ip = [];
            for (let i = 0; i < over.length; i++) {
                for (let j = 0; j < under.length; j++) {
                    ip.push(intersectZ(over[i], under[j], z));
                }
            }
            return ip;
        }

        /**
         * Ensure points are unique with a cache/key algorithm
         */
        function getCachedPoint(phash, p) {
            let cached = phash[p.key];
            if (!cached) {
                phash[p.key] = p;
                return p;
            }
            return cached;
        }
        /**
         * Given two points and hints about their edges,
         * return a new Line object with points sorted
         * lexicographically by key.  This allows for future
         * line de-duplication and joins.
         *
         * @param {Object} phash
         * @param {Point} p1
         * @param {Point} p2
         * @param {boolean} [coplanar]
         * @param {boolean} [edge]
         * @returns {Line}
         */
        function makeZLine(phash, p1, p2, coplanar, edge) {
            p1 = getCachedPoint(phash, p1);
            p2 = getCachedPoint(phash, p2);
            let line = newOrderedLine(p1,p2);
            line.coplanar = coplanar || false;
            line.edge = edge || false;
            return line;
        }

        /**
         * process a single z-slice on a single mesh and
         * add to slices array
         *
         * @param {number} z
         * @param {number} [height] optional real height (fdm)
         */
        function sliceZScrap(z, height, onflat, online, isScrap) {
            if (isScrap)
            {
                console.log("slicingScrap");
            }
            let phash = {},
                lines = [],
                slice = newSlice(z, options.view ? options.view.newGroup() : null),
                bucket = bucketCount == 1 ? points : buckets[Math.floor(z * zScale)];

            if (!bucket) {
                return;
            }

            // annotate slices with cam flats for finishing waterlines
            if (onflat && isCam) {
                slice.hasFlats = true;
            }

            // iterate over matching buckets for this z offset
            for (let i = 0; i < bucket.length;) {
                p1 = bucket[i++];
                p2 = bucket[i++];
                p3 = bucket[i++];
                let where = {under: [], over: [], on: []};
                checkUnderOverOn(p1, z, where);
                checkUnderOverOn(p2, z, where);
                checkUnderOverOn(p3, z, where);
                if (where.under.length === 3 || where.over.length === 3) {
                    // does not intersect (all 3 above or below)
                } else if (where.on.length === 2) {
                    // one side of triangle is on the Z plane
                    lines.push(makeZLine(phash, where.on[0], where.on[1], false, true));
                } else if (where.on.length === 3) {
                    // triangle is coplanar with Z
                    // we drop these because this face is attached to 3 others
                    // that will satisfy the if above (line) with 2 points
                } else if (where.under.length === 0 || where.over.length === 0) {
                    // does not intersect but one point is on the slice Z plane
                } else {
                    // compute two point intersections and construct line
                    let line = intersectPoints(where.over, where.under, z);
                    if (line.length < 2 && where.on.length === 1) {
                        line.push(where.on[0]);
                    }
                    if (line.length === 2) {
                        lines.push(makeZLine(phash, line[0], line[1]));
                    } else {
                        console.log({msg: "invalid ips", line: line, where: where});
                    }
                }
            }

            // allow empty slices in CAM swap mode (for topos w/ gaps)
            // if (lines.length == 0 && !(options.swapX || options.swapY)) return;
            if (lines.length == 0 && options.noEmpty) return;

            slice.height = height;
            slice.index = slices.length;
            slice.lines = removeDuplicateLines(lines);

            // for topo slices, we just need the raw lines
            if (!topoMode && !simpleMode) {
                slice.groups = connectLines(slice.lines, slices.length);
                // if (isScrap) {
                //     POLY.nest(slice.groups).forEach(function(scrap) { slice.addScrap(scrap) });
                // }
                // else {
                POLY.nest(slice.groups).forEach(function(top) { slice.addTop(top) });
                // }
                
            }

            // fixup un-rotates polygons for CAM
            if (options.swapX || options.swapY) {
                let move = {x:ox, y:oy, z:0};
                slice.camMode = options.swapX ? CPRO.FINISH_X : CPRO.FINISH_Y;
                if (topoMode) {
                    let lines = slice.lines, llen = lines.length, idx, line;
                    // shared points causing problems
                    for (idx=0; idx<llen; idx++) {
                        line = lines[idx];
                        line.p1 = line.p1.clone();
                        line.p2 = line.p2.clone();
                    }
                    for (idx=0; idx<llen; idx++) {
                        line = lines[idx];
                        if (options.swapX) {
                            line.p1.swapXZ();
                            line.p2.swapXZ();
                        } else {
                            line.p1.swapYZ();
                            line.p2.swapYZ();
                        }
                        line.p1.move(move);
                        line.p2.move(move);
                    }
                } else if (simpleMode) {
                    // fdm polishing mode
                    slice.groups = connectLines(slice.lines, slices.length);
                    slice.groups.forEach(poly => {
                        poly.swap(options.swapX, options.swapY);
                        poly.move(move);
                        poly.inner = null;
                    });
                }
            }
            slices.push(slice);
            return slice;
        }
    }

    /**
     * Given an array of points as triples, a bounding box and a set of
     * slicing controls, emit an array of Slice objects to the ondone()
     * function. onupdate() will be called with two parameters (% completion
     * and an optional message) so that the UI can report progress to the user.
     *
     * @param {Array} points vertex array
     * @param {Bounds} bounds bounding box for points
     * @param {Object} options slicing parameters
     * @param {Function} ondone callback when slicing done
     * @param {Function} onupdate callback to report slicing progress
     */
    function slice(points, bounds, options, ondone, onupdate, ondoneScrap, onupdateScrap, isScrap, scrapData) {
        // widget.loadFromCatalog()
        // console.log("SLICER_slice_Widget", scrapData);
        // platform.load_stl("/obj/cube.stl", function(vert) {
        //     CATALOG.putFile("sample cube.stl", vert);
        //     platform.compute_max_z();
        //     SPACE.view.home();
        //     setTimeout(API.space.save,500);
        //     then();
        //     API.help.show();
        // });

        // new MOTO.STL().load(url, function(vertices) {
        //     platform.add(newWidget().loadVertices(vertices));
        //     if (onload) onload(vertices);
        // })


        // var msglist = DOC.getElementById("msglist");

        // var show = msglist.getAttribute("data-list-size");
        // msglist.setAttribute("data-list-size", +show+3);
        // console.log("msglist", msglist);
        // console.log("show", show);

        // console.log(KIRI);
        // console.log("slicer-ScrapDataArray", ScrapDataArray);

        let debugging = false;

        if (debugging == true) {
            console.log("slicer_slicing");
            console.log("scrapData", scrapData);
            console.log("isScrap", isScrap);
        }
        let scrapSlices = [];
        if (scrapData[1]) {
            scrapSlices = sliceScrap(scrapData[1].points, scrapData[1].bounds, options, ondoneScrap, onupdateScrap, isScrap);
        }
        
        if (debugging == true) {
            console.log("Scrapslices:", scrapSlices);
        }
        

        let topoMode = options.topo,
            simpleMode = options.simple,
            swap = options.swapX || options.swapY,
            isCam = options.cam,
            ox = 0,
            oy = 0;

        // handle rotating meshes for CAM finishing.
        // slicer expects things just so, so we alter
        // geometry to satisfy
        if (swap) {
            points = points.slice();

            let btmp = new THREE.Box3(),
                pref = {},
                cached;

            btmp.setFromPoints(points);
            if (options.swapX) ox = -btmp.max.x;
            if (options.swapY) oy = -btmp.max.y;

            // array re-uses points so we need
            // to be careful not to alter a point
            // more than once
            for (let p, index=0; index<points.length; index++) {
                p = points[index];
                cached = pref[p.key];
                // skip points already altered
                if (cached) {
                    points[index] = cached;
                    continue;
                }
                cached = p.clone();
                if (options.swapX) cached.swapXZ();
                else if (options.swapY) cached.swapYZ();
                cached.rekey();
                pref[p.key] = cached;
                points[index] = cached;
            }

            // update temp bounds from new points
            btmp.setFromPoints(points);
            for (let p, index=0; index<points.length; index++) {
                p = points[index];
                if (p.mod === 1) continue;
                p.mod = 1;
                p.z -= btmp.min.z;
            }

            // update temp bounds from points with altered Z
            btmp.setFromPoints(points);
            bounds = btmp;
        }

        let zMin = options.zmin || Math.floor(bounds.min.z),
            zMax = options.zmax || Math.ceil(bounds.max.z),
            zInc = options.height,
            zIncMin = options.minHeight,
            zIncFirst = options.firstHeight || zInc,
            zOff = true ? zInc / 2 : 0,
            zHeights = [],      // heights for zIndexes in adaptive mode
            zIndexes = [],      // auto-detected z slicing offsets (laser/cam)
            zOrdered = [],      // ordered list of Z indexes
            zList = {},         // map count of z index points for adaptive slicing
            zFlat = {},         // map area of z index flat areas (cam)
            zLines = {},        // map count of z index lines
            zScale,             // bucket span in z units
            timeStart = time(),
            slices = [],
            zSum = 0.0,
            buckets = [],
            scrapBuckets = [],
            i, j = 0, k, p1, p2, p3, px,
            CPRO = KIRI.driver.CAM.process;

        if (options.add) zMax += zInc;

        function countZ(z) {
            z = UTIL.round(z,5);
            zList[z] = (zList[z] || 0) + 1;
        }

        // gather z-index stats
        // these are used for auto-slicing in laser
        // and to flats detection in CAM mode
        for (i = 0; i < points.length;) {
            p1 = points[i++];
            p2 = points[i++];
            p3 = points[i++];
            zSum += (Math.abs(p1.z - p2.z) + Math.abs(p2.z - p3.z) + Math.abs(p3.z - p1.z));
            // laser auto-detect z slice points
            if (zInc === 0 || zIncMin) {
                countZ(p1.z);
                countZ(p2.z);
                countZ(p3.z);
            }
            // use co-flat and co-line detection to adjust slice Z
            if (p1.z === p2.z && p2.z === p3.z && p1.z > bounds.min.z) {
                // detect zFlat faces to avoid slicing directly on them
                let zkey = p1.z.toFixed(5),
                    area = Math.abs(UTIL.area2(p1,p2,p3)) / 2;
                if (!zFlat[zkey]) {
                    zFlat[zkey] = area;
                } else {
                    zFlat[zkey] += area;
                }
            } else if (true || options.trace) {
                // detect zLines (curved region tops/bottoms)
                // mark these layers for ball and v mill tracing
                if (p1.z === p2.z && p1.z > bounds.min.z) {
                    let zkey = p1.z.toFixed(5);
                    let zval = zLines[zkey];
                    zLines[zkey] = (zval || 0) + 1;
                }
                if (p2.z === p3.z && p2.z > bounds.min.z) {
                    let zkey = p2.z.toFixed(5);
                    let zval = zLines[zkey];
                    zLines[zkey] = (zval || 0) + 1;
                }
                if (p3.z === p1.z && p3.z > bounds.min.z) {
                    let zkey = p3.z.toFixed(5);
                    let zval = zLines[zkey];
                    zLines[zkey] = (zval || 0) + 1;
                }
            }
        }

        // CAM: find slice candidates to trace for ballmills and tapermills
        if (options.trace) {
            let zl = {};
            let le;
            let zs = Object.entries(zLines).map(oe => {
                return oe.map(v => parseFloat(v));
            }).sort((a,b) => {
                return b[0] - a[0];
            }).forEach((e,i) => {
                if (i > 0) {
                    let zd = le[0]-e[0];
                    if (zd > 0.1 && e[1] > 100) {
                        // zl.push(e)
                        zl[e[0].toFixed(5)] = e[1];
                        zFlat[e[0].toFixed(5)] = e[1];
                    }
                }
                if (e[1] > 10) {
                    le = e;
                }
            });
            zLines = zl;
        }

        /**
         * bucket polygons into z-bounded groups (inside or crossing)
         * to reduce the search space in complex models
         */
        let bucketCount = Math.max(1, Math.ceil(zMax / (zSum / points.length)) - 1);

        zScale = 1 / (zMax / bucketCount);

        if (bucketCount > 1) {
            // create empty buckets
            for (i = 0; i < bucketCount + 1; i++) buckets.push([]);

            // copy triples into all matching z-buckets
            for (i = 0; i < points.length;) {
                p1 = points[i++];
                p2 = points[i++];
                p3 = points[i++];
                let zm = Math.min(p1.z, p2.z, p3.z),
                    zM = Math.max(p1.z, p2.z, p3.z),
                    bm = Math.floor(zm * zScale),
                    bM = Math.ceil(zM * zScale);
                for (j = bm; j < bM; j++) {
                    buckets[j].push(p1);
                    buckets[j].push(p2);
                    buckets[j].push(p3);
                }
            }
        }

        // we need Z ordered list for laser auto or adaptive fdm slicing
        if (zInc === 0 || zIncMin) {
            for (let key in zList) {
                if (!zList.hasOwnProperty(key)) continue;
                zOrdered.push(parseFloat(key));
            }
            zOrdered.sort(function(a,b) { return a - b});
        }

        if (options.single) {
            // usually for laser single slice
            zIndexes.push(zMin + zInc);
        } else if (zInc === 0) {
            // use Z indices in auto slice mode for laser
            // find unique z-index offsets for slicing
            let zl = zOrdered
            for (i = 0; i < zl.length - 1; i++) {
                zIndexes.push((zl[i] + zl[i+1]) / 2);
            }
        } else if (isCam) {
            // re-divide slice height so that top and
            // bottom slices fall exactly on those faces
            zInc = (zMax - zMin) / (Math.floor(zMax / zInc) + 1);
            for (i = zMin; i < zMax; i += zInc) {
                zIndexes.push(i);
            }
            for (let key in zFlat) {
                // todo make threshold for flat detection configurable
                if (!zFlat.hasOwnProperty(key) || zFlat[key] < 10){
                    continue;
                }
                key = parseFloat(key);
                if (!zIndexes.contains(key) && key >= zMin) {
                    zIndexes.push(key);
                }
            }
            // sort top down
            zIndexes.sort(function(a,b) {
                return b-a;
            });
            // copy to store original z
            zHeights = zIndexes.slice();
        } else if (zIncMin) {
            // FDM adaptive slicing
            let zPos = zMin + zIncFirst,
                zOI = 0,
                zDelta,
                zDivMin,
                zDivMax,
                zStep,
                nextZ,
                lzp = zPos;

            // first slice is fixed
            zHeights.push(zIncFirst);
            zIndexes.push(zIncFirst / 2);

            while (zPos < zMax && zOI < zOrdered.length) {
                nextZ = zOrdered[zOI++];
                if (zPos >= nextZ) continue;
                zDelta = nextZ - zPos;
                if (zDelta < zIncMin) continue;

                zDivMin = Math.floor(zDelta / zIncMin);
                zDivMax = Math.floor(zDelta / zInc);

                if (zDivMax && zDivMax <= zDivMin) {
                    if (zDelta % zInc > 0.01) zDivMax++;
                    zStep = zDelta / zDivMax;
                    // console.log(`--- zDivMax <= zDivMin ---`, zStep, zDelta % zInc)
                } else {
                    zStep = zDelta;
                }
                // console.log({nextZ, zPos, zDelta, zStep, zDivMin, zDivMax})
                while (zPos < nextZ) {
                    zHeights.push(zStep);
                    zIndexes.push(zPos + zStep / 2);
                    zPos += zStep;
                    // console.log({D: zPos - lzp, zPos})
                    // lzp = zPos;
                }
            }
        } else {
            // FDM fixed slicing
            if (options.firstHeight) {
                zIndexes.push(options.firstHeight / 2);
                zHeights.push(options.firstHeight);
                zMin = options.firstHeight; 
            }
            for (i = zMin + zOff; i < zMax; i += zInc) {
                zIndexes.push(i);
                zHeights.push(zInc);
            }
        }

        // TODO LWW: Doing it this way is faster but assumes no slices without tops in the scrapSlices once scrap started 
        scrapSlices = scrapSlices.filter(slice => slice.tops.length);
        let scrapStartIndex = 0;
        let scrapStartFound = false;
        if (scrapSlices.length >= 1) {
            for (let i = 0; i < zIndexes.length; i++) { 
                // if within lowest scrap layer is within layer height of current layer, scrap starts here
                if ((zIndexes[i] - (zInc / 2)) <= scrapSlices[0].z && (zIndexes[i] + (zInc / 2)) >= scrapSlices[0].z) {
                    scrapStartIndex = i;
                    scrapStartFound = true;
                    break;
                }
            }
        }
        if (debugging == true) {
            console.log("scrapStart found:", scrapStartFound, scrapStartIndex);
        }

        // create a Slice for each z offset in the zIndexes array
        for (let i = 0; i < zIndexes.length; i++) {
            let ik = zIndexes[i].toFixed(5),
                onFlat = false,
                onLine = false;
            // ensure no slice through horizontal lines or planes
            if (zFlat[ik]) onFlat = true;
            if (zLines[ik]) onLine = true;
            if (onFlat || onLine) {
                zIndexes[i] += isCam ? 0.001 : -0.001;
            }
            // slice next layer and add to slices[] array
            let slice = sliceZ(zIndexes[i], zHeights[i], onFlat, onLine, isScrap);

            // If within scrap height area, add scrap slice to model slice
            if (scrapStartFound) {
                if ((i - scrapStartIndex) >= 0 && (i - scrapStartIndex) < scrapSlices.length) {
                    slice.setScrapSlice(scrapSlices[i - scrapStartIndex]);
                    if (Math.abs(scrapSlices[i - scrapStartIndex].z - slice.z) > (zInc / 2)) {
                        console.log("MISALINGED SCRAP DETECTED");
                    }
                }
            }

            // override z in cam in case it was moved for a flat or line
            if (isCam && slice) {
                slice.z = zHeights[i];
            }
            onupdate(i / zIndexes.length);
        }
        if (debugging == true) {
            console.log("model slices", slices);
        }

        // for cam, bottom as mandatory (hasFlats)
        if (isCam && slices.length > 0) {
            // slices[0].hasFlats = true;
            slices[slices.length-1].hasFlats = true;
        }

        // connect slices into linked list for island/bridge projections
        for (i=1; i<slices.length; i++) {
            slices[i-1].up = slices[i];
            slices[i].down = slices[i-1];
        }

        slices.slice_time = time() - timeStart;

        // pass Slices array back to ondone function
        ondone(slices);

        /** ***** SLICING FUNCTIONS ***** */

        /**
         * given a point, append to the correct
         * 'where' objec tarray (on, over or under)
         *
         * @param {Point} p
         * @param {number} z offset
         * @param {Obejct} where
         */
        function checkUnderOverOn(p, z, where) {
            let delta = p.z - z;
            if (Math.abs(delta) < CONF.precision_slice_z) { // on
                where.on.push(p);
            } else if (delta < 0) { // under
                where.under.push(p);
            } else { // over
                where.over.push(p);
            }
        }

        /**
         * Given a point over and under a z offset, calculate
         * and return the intersection point on that z plane
         *
         * @param {Point} over
         * @param {Point} under
         * @param {number} z offset
         * @returns {Point} intersection point
         */
        function intersectPoints(over, under, z) {
            let ip = [];
            for (let i = 0; i < over.length; i++) {
                for (let j = 0; j < under.length; j++) {
                    ip.push(over[i].intersectZ(under[j], z));
                }
            }
            return ip;
        }

        /**
         * Ensure points are unique with a cache/key algorithm
         */
        function getCachedPoint(phash, p) {
            let cached = phash[p.key];
            if (!cached) {
                phash[p.key] = p;
                return p;
            }
            return cached;
        }
        /**
         * Given two points and hints about their edges,
         * return a new Line object with points sorted
         * lexicographically by key.  This allows for future
         * line de-duplication and joins.
         *
         * @param {Object} phash
         * @param {Point} p1
         * @param {Point} p2
         * @param {boolean} [coplanar]
         * @param {boolean} [edge]
         * @returns {Line}
         */
        function makeZLine(phash, p1, p2, coplanar, edge) {
            p1 = getCachedPoint(phash, p1);
            p2 = getCachedPoint(phash, p2);
            let line = newOrderedLine(p1,p2);
            line.coplanar = coplanar || false;
            line.edge = edge || false;
            return line;
        }

        /**
         * process a single z-slice on a single mesh and
         * add to slices array
         *
         * @param {number} z
         * @param {number} [height] optional real height (fdm)
         */
        function sliceZ(z, height, onflat, online, isScrap) {
            if (isScrap && debugging == true)
            {
                console.log("slicingScrap");
            }
            let phash = {},
                lines = [],
                slice = newSlice(z, options.view ? options.view.newGroup() : null),
                bucket = bucketCount == 1 ? points : buckets[Math.floor(z * zScale)];

            if (!bucket) {
                return;
            }

            // annotate slices with cam flats for finishing waterlines
            if (onflat && isCam) {
                slice.hasFlats = true;
            }

            // iterate over matching buckets for this z offset
            for (let i = 0; i < bucket.length;) {
                p1 = bucket[i++];
                p2 = bucket[i++];
                p3 = bucket[i++];
                let where = {under: [], over: [], on: []};
                checkUnderOverOn(p1, z, where);
                checkUnderOverOn(p2, z, where);
                checkUnderOverOn(p3, z, where);
                if (where.under.length === 3 || where.over.length === 3) {
                    // does not intersect (all 3 above or below)
                } else if (where.on.length === 2) {
                    // one side of triangle is on the Z plane
                    lines.push(makeZLine(phash, where.on[0], where.on[1], false, true));
                } else if (where.on.length === 3) {
                    // triangle is coplanar with Z
                    // we drop these because this face is attached to 3 others
                    // that will satisfy the if above (line) with 2 points
                } else if (where.under.length === 0 || where.over.length === 0) {
                    // does not intersect but one point is on the slice Z plane
                } else {
                    // compute two point intersections and construct line
                    let line = intersectPoints(where.over, where.under, z);
                    if (line.length < 2 && where.on.length === 1) {
                        line.push(where.on[0]);
                    }
                    if (line.length === 2) {
                        lines.push(makeZLine(phash, line[0], line[1]));
                    } else {
                        console.log({msg: "invalid ips", line: line, where: where});
                    }
                }
            }

            // allow empty slices in CAM swap mode (for topos w/ gaps)
            // if (lines.length == 0 && !(options.swapX || options.swapY)) return;
            if (lines.length == 0 && options.noEmpty) return;

            slice.height = height;
            slice.index = slices.length;
            slice.lines = removeDuplicateLines(lines);

            // for topo slices, we just need the raw lines
            if (!topoMode && !simpleMode) {
                slice.groups = connectLines(slice.lines, slices.length);
                // if (isScrap) {
                //     POLY.nest(slice.groups).forEach(function(scrap) { slice.addScrap(scrap) });
                // }
                // else {
                POLY.nest(slice.groups).forEach(function(top) { slice.addTop(top) });
                // }
                
            }

            // fixup un-rotates polygons for CAM
            if (options.swapX || options.swapY) {
                let move = {x:ox, y:oy, z:0};
                slice.camMode = options.swapX ? CPRO.FINISH_X : CPRO.FINISH_Y;
                if (topoMode) {
                    let lines = slice.lines, llen = lines.length, idx, line;
                    // shared points causing problems
                    for (idx=0; idx<llen; idx++) {
                        line = lines[idx];
                        line.p1 = line.p1.clone();
                        line.p2 = line.p2.clone();
                    }
                    for (idx=0; idx<llen; idx++) {
                        line = lines[idx];
                        if (options.swapX) {
                            line.p1.swapXZ();
                            line.p2.swapXZ();
                        } else {
                            line.p1.swapYZ();
                            line.p2.swapYZ();
                        }
                        line.p1.move(move);
                        line.p2.move(move);
                    }
                } else if (simpleMode) {
                    // fdm polishing mode
                    slice.groups = connectLines(slice.lines, slices.length);
                    slice.groups.forEach(poly => {
                        poly.swap(options.swapX, options.swapY);
                        poly.move(move);
                        poly.inner = null;
                    });
                }
            }
            slices.push(slice);
            return slice;
        }
    }

    /**
     * Given an array of input lines (line soup), find the path through
     * joining line ends that encompasses the greatest area without self
     * interesection.  Eliminate used points and repeat.  Unjoined lines
     * are permitted and handled after all other cases are handled.
     *
     * @param {Line[]} input
     * @param {number} [index]
     * @returns {Array}
     */
    function connectLines(input) {
        // map points to all other points they're connected to
        let DBUG = BASE.debug,
            CONF = BASE.config,
            pmap = {},
            points = [],
            output = [],
            connect = [],
            search = 1,
            nextMod = 1,
            bridge = CONF.bridgeLineGapDistance,
            p1, p2;

        function cachedPoint(p) {
            let cp = pmap[p.key];
            if (cp) return cp;
            points.push(p);
            pmap[p.key] = p;
            p.mod = nextMod++; // unique seq ID for points
            p.toString = function() { return this.mod }; // point array concat
            return p;
        }

        function addConnected(p1, p2) {
            if (!p1.group) p1.group = [ p2 ];
            else p1.group.push(p2);
        }

        function sliceAtTerm(path, term) {
            let idx, len = path.length;
            for (idx = 0; idx < len-1; idx++) {
                if (path[idx] === term) {
                    return path.slice(idx);
                }
            }
            return path;
        }

        /**
         * using minimal recursion, follow points through connected lines
         * to form candidate output paths.
         */
        function findPathsMinRecurse(point, path, paths, from) {
            let stack = [ ];
            if (paths.length > 10000) {
                DBUG.log("excessive path options @ "+paths.length+" #"+input.length);
                return;
            }
            for (;;) {
                stack.push(point);

                let last = point,
                    links = point.group;

                path.push(point);
                // use del to mark traversed path
                point.del = true;
                // set so point isn't used in another polygon search
                point.pos = search++;
                // seed path with two points to prevent redundant opposing seeks
                if (path.length === 1) {
                    from = point;
                    point = links[0];
                    continue;
                }

                if (links.length > 2) {
                    // TODO optimize when > 2 and limit to left-most and right-most branches
                    // for now, pursue all possible branches
                    links.forEach(function(nextp) {
                        // do not backtrack
                        if (nextp === from) {
                            return;
                        }
                        if (nextp.del) {
                            paths.push(sliceAtTerm(path,nextp));
                        } else {
                            findPathsMinRecurse(nextp, path.slice(), paths, point);
                        }
                    });
                    break;
                } else {
                    point = links[0] === from ? links[1] : links[0];
                    from = last;
                    // hit an open end
                    if (!point) {
                        path.open = true;
                        paths.push(path);
                        break;
                    }
                    // hit a point previously in the path (or start)
                    if (point.del) {
                        paths.push(sliceAtTerm(path,point));
                        break;
                    }
                }
            }

            for (let i=0; i<stack.length; i++) stack[i].del = false;
            // stack.forEach(function(p) { p.del = false });
        }

        // emit a polygon if it can be cleaned and still have 2 or more points
        function emit(poly) {
            poly = poly.clean();
            if (poly.length > 2) output.push(poly);
        }

        // given an array of paths, emit longest to shortest
        // eliminating points from the paths as they are emitted
        // shorter paths any point eliminated are eliminated as candidates.
        function emitLongestAsPolygon(paths) {
            let longest = null,
                emitted = 0,
                closed = 0,
                open = 0;

            paths.forEach(function(path) {
                // use longest perimeter vs longest path?
                if (!longest || path.length > longest.length) longest = path;
                if (!path.open) closed++; else open++;
            });

            // it gets more complicated with multiple possible output paths
            if (closed > 1 && open === 0) {
                // add polygon to path (for area sorting)
                paths.forEach(function(path) { path.poly = BASE.newPolygon().addPoints(path) });

                // sort descending by area VS (length below -- better in most cases)
                // paths.sort(function(a,b) { return b.poly.area() - a.poly.area() });

                // sort descending by length
                paths.sort(function(a,b) { return b.poly.length - a.poly.length });

                // emit polygons largest to smallest
                // omit polygon if it intersects previously emitted (has del points)
                paths.forEach(function(path) {
                    if (path.length < 3) return;
                    let len = path.length, i;
                    for (i = 0; i < len; i++) if (path[i].del) return;
                    for (i = 0; i < len; i++) path[i].del = true;
                    emit(path.poly);
                    emitted++;
                });
            } else {
                if (longest.open) {
                    connect.push(longest);
                } else {
                    emit(BASE.newPolygon().addPoints(longest));
                }
            }
        }

        // create point map, unique point list and point group arrays
        input.forEach(function(line) {
            p1 = cachedPoint(line.p1.round(7));
            p2 = cachedPoint(line.p2.round(7));
            addConnected(p1,p2);
            addConnected(p2,p1);
        });

        // first trace paths starting at dangling endpoinds (bad polygon soup)
        points.forEach(function(point) {
            // must not have been used and be a dangling end
            if (point.pos === 0 && point.group.length === 1) {
                let path = [],
                    paths = [];
                findPathsMinRecurse(point, path, paths);
                if (paths.length > 0) emitLongestAsPolygon(paths);
            }
        });

        // for each point, find longest path back to self
        points.forEach(function(point) {
            // must not have been used or be at a split
            if (point.pos === 0 && point.group.length === 2) {
                let path = [],
                    paths = [];
                findPathsMinRecurse(point, path, paths);
                if (paths.length > 0) emitLongestAsPolygon(paths);
            }
        });

        // return true if points are deemed "close enough" close a polygon
        function close(p1,p2) {
            return p1.distToSq2D(p2) <= 0.01;
        }

        // reconnect dangling/open polygons to closest endpoint
        for (let i=0; i<connect.length; i++) {

            let array = connect[i],
                last = array[array.length-1],
                tmp, dist, j;

            if (!bridge) {
                emit(BASE.newPolygon().addPoints(array).setOpen());
                continue;
            }

            if (array.delete) continue;

            loop: for (let merged=0;;) {
                let closest = { dist:Infinity };
                for (j=i+1; j<connect.length; j++) {
                    tmp = connect[j];
                    if (tmp.delete) continue;
                    dist = last.distToSq2D(tmp[0]);
                    if (dist < closest.dist && dist <= bridge) {
                        closest = {
                            dist: dist,
                            array: tmp
                        }
                    }
                    dist = last.distToSq2D(tmp[tmp.length-1]);
                    if (dist < closest.dist && dist <= bridge) {
                        closest = {
                            dist: dist,
                            array: tmp,
                            reverse: true
                        }
                    }
                }

                if (tmp = closest.array) {
                    if (closest.reverse) tmp.reverse();
                    tmp.delete = true;
                    array.appendAll(tmp);
                    last = array[array.length-1];
                    merged++;
                    // tail meets head (closed)
                    if (close(array[0], last)) {
                        emit(BASE.newPolygon().addPoints(array));
                        break loop;
                    }
                } else {
                    // no more closest polys (open set)
                    emit(BASE.newPolygon().addPoints(array));
                    break loop;
                }
            }
        }

        return output;
    }

    /**
     * eliminate duplicate lines and interior-only lines (coplanar)
     *
     * lines are sorted using lexicographic point keys such that
     * they are comparable even if their points are reversed. hinting
     * for deletion, co-planar and suspect shared edge is detectable at
     * this time.
     *
     * @param {Line[]} lines
     * @returns {Line[]}
     */
    function removeDuplicateLines(lines) {
        let output = [],
            tmplines = [],
            points = [],
            pmap = {};

        function cachePoint(p) {
            let cp = pmap[p.key];
            if (cp) return cp;
            points.push(p);
            pmap[p.key] = p;
            return p;
        }

        function addLinesToPoint(point, line) {
            cachePoint(point);
            if (!point.group) point.group = [ line ];
            else point.group.push(line);
        }

        // mark duplicates for deletion preserving edges
        lines.sort(function (l1, l2) {
            if (l1.key === l2.key) {
                l1.del = !l1.edge;
                l2.del = !l2.edge;
                return 0;
            }
            return l1.key < l2.key ? -1 : 1;
        });

        // associate points with their lines, cull deleted
        lines.forEach(function(line) {
            if (!line.del) {
                tmplines.push(line);
                addLinesToPoint(line.p1, line);
                addLinesToPoint(line.p2, line);
            }
        });

        // merge collinear lines
        points.forEach(function(point) {
            if (point.group.length != 2) return;
            let l1 = point.group[0],
                l2 = point.group[1];
            if (l1.isCollinear(l2)) {
                l1.del = true;
                l2.del = true;
                // find new endpoints that are not shared point
                let p1 = l1.p1 != point ? l1.p1 : l1.p2,
                    p2 = l2.p1 != point ? l2.p1 : l2.p2,
                    newline = base.newOrderedLine(p1,p2);
                // remove deleted lines from associated points
                p1.group.remove(l1);
                p1.group.remove(l2);
                p2.group.remove(l1);
                p2.group.remove(l2);
                // associate new line with points
                p1.group.push(newline);
                p2.group.push(newline);
                // add new line to lines array
                newline.edge = l1.edge || l2.edge;
                tmplines.push(newline);
            }
        });

        // mark duplicates for deletion
        // but preserve one if it's an edge
        tmplines.sort(function (l1, l2) {
            if (l1.key === l2.key) {
                l1.del = true;
                l2.del = !l2.edge;
                return 0;
            }
            return l1.key < l2.key ? -1 : 1;
        });

        // create new line array culling deleted
        tmplines.forEach(function(line) {
            if (!line.del) {
                output.push(line);
                line.p1.group = null;
                line.p2.group = null;
            }
        });

        return output;
    }

})();
