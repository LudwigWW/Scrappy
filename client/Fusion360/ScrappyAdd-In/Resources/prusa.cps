/**
  Copyright (C) 2018-2020 by Autodesk, Inc.
  All rights reserved.

  3D additive printer post configuration.

  $Revision: 42686 e4a29d194e029389d39090a414c00d5a520e055b $
  $Date: 2020-03-13 15:20:24 $

  FORKID {FEF3D431-E19A-4541-8E5C-A32A33348505}
*/

description = "Prusa FFF Machine";
vendor = "Prusa";
vendorUrl = "https://www.prusa3d.com/";
legal = "Copyright (C) 2012-2020 by Autodesk, Inc.";
certificationLevel = 2;
minimumRevision = 45633;

longDescription = "Post to export toolpath for the Prusa i3 family or mini FFF printer in gcode format (i3, i3 MK2, i3 MK3 ..., mini)";

extension = "gcode";
setCodePage("ascii");

capabilities = CAPABILITY_ADDITIVE;
tolerance = spatial(0.002, MM);
highFeedrate = (unit == MM) ? 6000 : 236;

// needed for range checking, will be effectively passed from Fusion
var printerLimits = {
  x: {min: 0, max: 250.0}, //Defines the x bed size
  y: {min: 0, max: 210.0}, //Defines the y bed size
  z: {min: 0, max: 210.0} //Defines the z bed size
};

// For information only
var bedCenter = {
  x: 0.0,
  y: 0.0,
  z: 0.0
};

var extruderOffsets = [[0, 0, 0], [0, 0, 0]];
var activeExtruder = 0;  //Track the active extruder.

// user-defined properties
properties = {
  printerModel: "i3mk2mk3",
  pauseLayerNumber: [103],
};

// user-defined property definitions
propertyDefinitions = {
  printerModel: {
    title: "Printer model",
    description: "Select the printer model for generating the gcode.",
    type: "enum",
    values:[
      {title:"i3, i3 XL", id:"i3i3xl"},
      {title:"i3 Mk2, i3 Mk3", id:"i3mk2mk3"},
      {title:"mini", id:"mini"}
    ]
  },
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
var dimensionFormat = createFormat({decimals: (unit == MM ? 3 : 4), zeropad: false, suffix: (unit == MM ? "mm" : "in")});

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

  writeComment("Printer Name: " + machineConfiguration.getVendor() + " " + machineConfiguration.getModel());
  writeComment("Print time: " + xyzFormat.format(printTime) + "s");
  writeComment("Material used: " + dimensionFormat.format(getExtruder(1).extrusionLength));
  writeComment("Material name: " + getExtruder(1).materialName);
  writeComment("Filament diameter: " + dimensionFormat.format(getExtruder(1).filamentDiameter));
  writeComment("Nozzle diameter: " + dimensionFormat.format(getExtruder(1).nozzleDiameter));
  writeComment("Extruder offset x: " + dimensionFormat.format(extruderOffsets[0][0]));
  writeComment("Extruder offset y: " + dimensionFormat.format(extruderOffsets[0][1]));
  writeComment("Extruder offset z: " + dimensionFormat.format(extruderOffsets[0][2]));
  writeComment("Max temp: " + integerFormat.format(getExtruder(1).temperature));
  writeComment("Bed temp: " + integerFormat.format(bedTemp));
  writeComment("Layer Count: " + integerFormat.format(layerCount));

  writeComment("width: " + dimensionFormat.format(printerLimits.x.max));
  writeComment("depth: " + dimensionFormat.format(printerLimits.y.max));
  writeComment("height: " + dimensionFormat.format(printerLimits.z.max));
  writeComment("center x: " + dimensionFormat.format(bedCenter.x));
  writeComment("center y: " + dimensionFormat.format(bedCenter.y));
  writeComment("center z: " + dimensionFormat.format(bedCenter.z));
  writeComment("Count of bodies: " + integerFormat.format(partCount));
  writeComment("Version of Fusion: " + getGlobalParameter("version", "0"));
  writeBlock("M115 U3.0.10 ; tell printer latest fw version");
  if ((properties.printerModel == "i3i3xl") || (properties.printerModel == "i3mk2mk3")) {
    writeBlock("G28 W ; home all without mesh bed level");
    writeBlock("G80 ; mesh bed leveling");
  }  else if (properties.printerModel == "mini") {
    writeBlock("G28 ; home all without mesh bed level");
    writeBlock("G29 ; mesh bed leveling");
  }
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
  writeBlock("M300 S1200 P100");
  writeBlock("M300 S0 P100");
  writeBlock("G4 ; wait");
  xOutput.reset();
  yOutput.reset();
  if ((properties.printerModel == "i3i3xl") || (properties.printerModel == "i3mk2mk3")) {
    writeBlock(gMotionModal.format(1), xOutput.format(0), yOutput.format((unit == MM ? 200 : 7.8)), "; home X axis");
  } else if (properties.printerModel == "mini") {
    writeBlock(gMotionModal.format(1), xOutput.format(0), yOutput.format((unit == MM ? 150 : 5.9)), "; home X axis");
  }
  writeBlock("M84 ; disable motors");
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

  writeBlock(gFormat.format(92), eOutput.format(0));

  // set unit
  writeBlock(gFormat.format(unit == MM ? 21 : 20));
  writeBlock(gAbsIncModal.format(90)); // absolute spatial co-ordinates
  writeBlock(mFormat.format(82)); // absolute extrusion co-ordinates
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
  if(properties.pauseLayerNumber.indexOf(num) == 0) {
      writeComment("Stopping now!");
      writeBlock("M300 S1200 P100 ; beep three times");
      writeBlock("M300 S0 P100");
      writeBlock("M300 S1200 P100");
      writeBlock("M300 S0 P100");
      writeBlock("M300 S1200 P100");
      writeBlock("M300 S0 P100");
      writeBlock("G1 X10.000 Y200.000 Z190.000; parking position for easy access");
      writeBlock("M1 Insert scrap now; user stop");
      //writeBlock("M105; return to current temp");
      writeBlock("M117 Insert scrap now");
  }
}

function onExtruderTemp(temp, wait, id) {
  var extruderString = "";

  if (properties.printerModel == "i3mk2mk3") {
    extruderString = tFormat.format(id);
  }
  if (id < numberOfExtruders) {
    if (wait) {
      writeBlock(mFormat.format(109), sOutput.format(temp), extruderString);
    } else {
      writeBlock(mFormat.format(104), sOutput.format(temp), extruderString);
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
