/**
  Copyright (C) 2018-2020 by Autodesk, Inc.
  All rights reserved.

  3D additive printer post configuration.

  $Revision: 42682 9471a22a72e5b6a8be7a1bf341716ac0009d9847 $
  $Date: 2020-03-09 03:36:19 $
  
  FORKID {02E4B33E-E4E1-4935-8810-D0B96FB7D0AA}
*/

description = "Ultimaker 2 FFF Machine";
vendor = "Ultimaker";
vendorUrl = "https://ultimaker.com/";
legal = "Copyright (C) 2012-2020 by Autodesk, Inc.";
certificationLevel = 2;
minimumRevision = 45633;

longDescription = "Post to export toolpath for then Ultimaker 2 family of printers in gcode format (2, 2+, 2 extended, 2 Go...)";

extension = "gcode";
setCodePage("ascii");

capabilities = CAPABILITY_ADDITIVE;
tolerance = spatial(0.002, MM);
highFeedrate = (unit == MM) ? 6000 : 236;

// needed for range checking, will be effectively passed from Fusion
var printerLimits = {
  x: {min: 0, max: 223.0}, //Defines the x bed size
  y: {min: 0, max: 223.0}, //Defines the y bed size
  z: {min: 0, max: 205.0} //Defines the z bed size
};

// For information only
var bedCenter = {
  x: 0.0,
  y: 0.0,
  z: 0.0
};

var extruderOffsets = [[0, 0, 0], [0, 0, 0]];
var activeExtruder = 0;  //Track the active extruder.

properties = {
  pauseLayerNumber: 6,
};

var xyzFormat = createFormat({decimals: (unit == MM ? 3 : 4)});
var xFormat = createFormat({decimals: (unit == MM ? 3 : 4)});
var yFormat = createFormat({decimals: (unit == MM ? 3 : 4)});
var zFormat = createFormat({decimals: (unit == MM ? 3 : 4)});
var gFormat = createFormat({prefix: "G", width: 1, zeropad: false, decimals: 0});
var mFormat = createFormat({prefix: "M", width: 2, zeropad: true, decimals: 0});
var tFormat = createFormat({prefix: "T", width: 1, zeropad: false, decimals: 0});
var feedFormat = createFormat({decimals: (unit == MM ? 0 : 1)});
var integerFormat = createFormat({decimals:0});

var gMotionModal = createModal({force: true}, gFormat); // modal group 1 // G0-G3, ...
var gPlaneModal = createModal({onchange: function () {gMotionModal.reset();}}, gFormat); // modal group 2 // G17-19 //Actually unused
var gAbsIncModal = createModal({}, gFormat); // modal group 3 // G90-91

var xOutput = createVariable({prefix: "X"}, xFormat);
var yOutput = createVariable({prefix: "Y"}, yFormat);
var zOutput = createVariable({prefix: "Z"}, zFormat);
var feedOutput = createVariable({prefix: "F"}, feedFormat);
var eOutput = createVariable({prefix: "E"}, xyzFormat);  // Extrusion length
var sOutput = createVariable({prefix: "S", force: true}, xyzFormat);  // Parameter temperature or speed

// Writes the specified block.
function writeBlock() {
  writeWords(arguments);
}

function onOpen() {
  getPrinterGeometry();

  if (programName) {
    writeComment(programName);
  }
  if (programComment) {
    writeComment(programComment);
  }

  writeBlock(gFormat.format(92), eOutput.format(0));

  // set unit
  writeBlock(gFormat.format(unit == MM ? 21 : 20));
  writeBlock(gAbsIncModal.format(90)); // absolute spatial co-ordinates
  writeBlock(mFormat.format(82)); // absolute extrusion co-ordinates

  var globalBoundaries = getSection(0).getBoundingBox();
  writeComment("FLAVOR:UltiGCode");
  writeComment("TIME:" + xyzFormat.format(printTime));
  writeComment("MATERIAL:" + xyzFormat.format(getExtruder(1).extrusionLength));
  writeComment("MATERIAL2:0");
  writeComment("NOZZLE_DIAMETER:" + xyzFormat.format(getExtruder(1).nozzleDiameter));
  writeComment("MINX:" + (xyzFormat.format(globalBoundaries.lower.x)));
  writeComment("MINY:" + (xyzFormat.format(globalBoundaries.lower.y)));
  writeComment("MINZ:" + (xyzFormat.format(globalBoundaries.lower.z)));
  writeComment("MAXX:" + (xyzFormat.format(globalBoundaries.upper.x)));
  writeComment("MAXY:" + (xyzFormat.format(globalBoundaries.upper.y)));
  writeComment("MAXZ:" + (xyzFormat.format(globalBoundaries.upper.z)));
}

function getPrinterGeometry() {
  machineConfiguration = getMachineConfiguration();

  // Get the printer geometry from the machine configuration
  printerLimits.x.min = 0 - machineConfiguration.getCenterPositionX();
  printerLimits.y.min = 0 - machineConfiguration.getCenterPositionY();
  printerLimits.z.min = 0 + machineConfiguration.getCenterPositionZ();

  printerLimits.x.max = machineConfiguration.getWidth() - machineConfiguration.getCenterPositionX();
  printerLimits.y.max = machineConfiguration.getDepth() - machineConfiguration.getCenterPositionY();
  printerLimits.z.max = machineConfiguration.getHeight() + machineConfiguration.getCenterPositionZ();

  //Can be used in the post for documenting purpose.
  bedCenter.x = (machineConfiguration.getWidth() / 2.0) - machineConfiguration.getCenterPositionX();
  bedCenter.y = (machineConfiguration.getDepth() / 2.0) - machineConfiguration.getCenterPositionY();
  bedCenter.z = machineConfiguration.getCenterPositionZ();

  //Get the extruder configuration
  extruderOffsets[0][0] = machineConfiguration.getExtruderOffsetX(1);
  extruderOffsets[0][1] = machineConfiguration.getExtruderOffsetY(1);
  extruderOffsets[0][2] = machineConfiguration.getExtruderOffsetZ(1);
  if (numberOfExtruders > 1) {
    extruderOffsets[1] = [];
    extruderOffsets[1][0] = machineConfiguration.getExtruderOffsetX(2);
    extruderOffsets[1][1] = machineConfiguration.getExtruderOffsetY(2);
    extruderOffsets[1][2] = machineConfiguration.getExtruderOffsetZ(2);
  }
}

function onClose() {
  writeComment("END OF GCODE");
}

function onComment(message) {
  writeComment(message);
}

function onSection() {
  var range = currentSection.getBoundingBox();
  axes = ["x", "y", "z"];
  formats = [xFormat, yFormat, zFormat];
  for (var element in axes) {
    var min = formats[element].getResultingValue(range.lower[axes[element]]);
    var max = formats[element].getResultingValue(range.upper[axes[element]]);
    if (printerLimits[axes[element]].max < max || printerLimits[axes[element]].min > min) {
      error(localize("A toolpath is outside of the build volume."));
    }
  }
}

function onRapid(_x, _y, _z) {
  var x = xOutput.format(_x);
  var y = yOutput.format(_y);
  var z = zOutput.format(_z);
  if (x || y || z) {
    writeBlock(gMotionModal.format(0), x, y, z);
  }
}

function onLinearExtrude(_x, _y, _z, _f, _e) {
  var x = xOutput.format(_x);
  var y = yOutput.format(_y);
  var z = zOutput.format(_z);
  var f = feedOutput.format(_f);
  var e = eOutput.format(_e);
  if (x || y || z || f || e) {
    writeBlock(gMotionModal.format(1), x, y, z, f, e);
  }
}

function onBedTemp(temp, wait) {
  if (wait) {
    writeBlock(mFormat.format(190), sOutput.format(temp));
  } else {
    writeBlock(mFormat.format(140), sOutput.format(temp));
  }
}

function onExtruderChange(id) {
  if (id < numberOfExtruders) {
    writeBlock(tFormat.format(id));
    activeExtruder = id;
    xOutput.reset();
    yOutput.reset();
    zOutput.reset();
  } else {
    error(localize("This printer doesn't support the extruder ") + integerFormat.format(id) + " !");
  }
}

function onExtrusionReset(length) {
  eOutput.reset();
  writeBlock(gFormat.format(92), eOutput.format(length));
}

function onLayer(num) {
  writeComment("Layer : " + integerFormat.format(num) + " of " + integerFormat.format(layerCount));
  if(num == properties.pauseLayerNumber) {
      writeComment("Stopping now!");
      writeBlock("G1 X10.000 Y200.000 Z190.000; parking position for easy access");
      writeBlock("M1 (Stop) Insert scrap now; user stop");
      writeBlock("M117 (Message) Insert scrap now");
  }
}

function onExtruderTemp(temp, wait, id) {
  if (id < numberOfExtruders) {
    if (wait) {
      writeBlock(mFormat.format(109), sOutput.format(temp), tFormat.format(id));
    } else {
      writeBlock(mFormat.format(104), sOutput.format(temp), tFormat.format(id));
    }
  } else {
    error(localize("This printer doesn't support the extruder ") + integerFormat.format(id) + " !");
  }
}

function onFanSpeed(speed, id) {
  // to do handle id information
  if (speed == 0) {
    writeBlock(mFormat.format(107));
  } else {
    writeBlock(mFormat.format(106), sOutput.format(speed));
  }
}

function onParameter(name, value) {
  switch (name) {
  //feedrate is set before rapid moves and extruder change
  case "feedRate":
    if (unit == IN) {
      value /= 25.4;
    }
    setFeedRate(value);
    break;
  //warning or error message on unhandled parameter?
  }
}

//user defined functions
function setFeedRate(value) {
  feedOutput.reset();
  writeBlock(gFormat.format(1), feedOutput.format(value));
}

function writeComment(text) {
  writeln(";" + text);
}

function getFormatedDate() {
  var d = new Date();
  var month = "" + (d.getMonth() + 1);
  var day = "" + d.getDate();
  var year = d.getFullYear();

  if (month.length < 2) {month = "0" + month;}
  if (day.length < 2) {day = "0" + day;}

  return [year, month, day].join("-");
}
