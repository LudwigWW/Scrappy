#Author-Ludwig Wilhelm Wall
#Description-Creates the Scrappy user interface and run-time logic

ports_in_use = []


import adsk.core, adsk.fusion, adsk.cam, traceback
import os, sys
import subprocess
import json
import queue
import threading
import time
import math
import datetime
from shutil import copyfile
from itertools import count

# sys.stdout = open("D:\\Temp\\console.txt", "w")

#get the path of add-in
scrappy_addin_path = os.path.dirname(os.path.realpath(__file__))
scrappy_library_path = scrappy_addin_path + "/Resources/Library/"
scrappy_library_full_path = scrappy_addin_path + "/Resources/LibraryFull/"
scrappy_library_simple_path = scrappy_addin_path + "/Resources/LibrarySimplified/"
scrappy_export_path = scrappy_addin_path + "/Resources/ExportedDesigns/"
scrappy_import_path = scrappy_addin_path + "/Resources/ImportedDesigns/"

print(scrappy_addin_path)

# sys.stdout = sys.__stdout__

#add the path to the searchable path collection
if not scrappy_addin_path in sys.path:
   sys.path.append(scrappy_addin_path) 

#from .scrappy_network import *
#import fusionClient #as fusionClient
from .scrappyClient import *

#from test import *

# if fusionClient not in sys.modules:
#     print('You have not imported the {} module'.format(fusionClient))

# Global list to keep all event handlers in scope.
handlers = []
gTimeline = type('timeline', (), {'markerPosition' : 0})() # global timeline, defined during run()
lastMakerPosition = 0
totalVolume = 0
_app = adsk.core.Application.cast(None)
_ui = adsk.core.UserInterface.cast(None)
num = 0
revisionCounter = 0
stopLayers = []
scrappyClient = scrappyClientClass("192.168.88.234", 47556, scrappy_library_path, scrappy_library_simple_path, scrappy_library_full_path, scrappy_export_path, scrappy_import_path, True)
exitFlag = 0
scrapList = []
modelList = []
autoGenerate = False

# TODO: Make second queue, so that mesh checks can't block preview checks, or start new thread for user-requests?
threadList = ["MT-1", "MT-2", "MT-3", "MT-4", "MT-5", "MT-6"]
queueLock = threading.Lock()
HTMLLock = threading.Lock()
revisionLock = threading.Lock()
unique = count()
workQueue = queue.PriorityQueue(0) # Infinite priority queue (Priority-Int, negative volume, unique, data)
# Priority 49: Create simplified files for sending
# Priority 50: Send file used for mesh generation
# Priority 100: User-requested Scrap position check
# Priority 200: User-requested Scrap mesh generation
# Priority 300: Scrap position check
# Priority 400: Scrap mesh generation

threads = []
layerHeight = 0.2

class modelObject(): # Data class keeping track of all info about models
    def __init__(self, revision, name, path):
        self.revision = revision
        self.name = name # filename
        self.blobName = self.name[:-9] + "_Blob.stl"
        self.simpleName = self.name[:-9] + "_Simp.stl"
        self.path = path
        self.blobPath = ""
        self.simplePath = ""

class scrapObject(): # Data class keeping track of all info about scrap objects
    def __init__(self, ID, path, imagePath, simplePath, fullPath, name, count):
        self.ID = ID
        self.originalPath = path
        self.path = fullPath
        self.simplePath = simplePath
        self.imagePath = imagePath
        self.originalName = name # filename # TODO: Replace name with fullName and remove originalName
        self.name = self.originalName[:-4] + "_Full.stl" 
        self.simpleName = self.originalName[:-4] + "_Simp.stl"
        self.modelName = ""
        self.modelObj = {}
        self.count = count # How many of these does the user have available
        self.isValid = False
        self.lastValidRevision = -1 # At what model-revision was this last checked to be a valid infill
        self.lastValidTime = -1 # At what time was this last checked to be a valid infill
        self.lastValidMeshTime = -1 # When was the model mesh requested 
        self.scrapAvailable = False
        self.modelAvailable = False
        # self.matryoshkaData = [] # Matryoshka data of last valid check
        # self.dataArrays = [] # pure redundancy?
        self.adjustedSTLPath = "" # Adjusted mesh of last valid check
        self.adjustedScrappedSTLPath = "" # mesh of the scrap including padding for insertion path
        self.adjustedModelSTLPath = "" # Holed-out mesh of parent model
        self.contFile = ""
        self.HTMLText = ""
        self.volume = 0
        self.stopHeight = math.inf
        self.stopLayer = math.inf
        self.retryCount = 0
        
        self.outerCenter = []
        self.innerCenter = []

        if not (os.path.isfile(self.simplePath)): # If simple STL does not exist yet
            # Parallel simplify
            simplifyDetails = type('obj', (object,), {'type':'Scrap', 'mode':'Simplify', 'send':False})()
            queueLock.acquire()
            addTime = datetime.datetime.now()
            newQueueItemDataBlob = queueItemData("simplify", self, revisionCounter, addTime, -1, {}, '', '', simplifyDetails) # no model object
            workQueue.put((49, self.volume, next(unique), newQueueItemDataBlob)) # simplified file needed later created in parallel
            queueLock.release()
            # createSimplifiedSTL(1750, self.originalPath, self.simplePath, self.originalName[:-4], 0)

        if not (os.path.isfile(self.path)):
            copyfile(self.originalPath, self.path)

    def __lt__(self, other):
        return self.volume < other.volume

    def __eq__(self, other):
        return self.volume == other.volume

    def updateDetails(self, dataArrays, newModelName, newModelObj, revision, addTime, meshTime, validity):
        for dataIndex in [1, 3, 5, 7, 11, 13, 15, 17, 18, 19, 20, 21]: #range(1, len(returnData[1]), 2):
            for numberIndex in range(0, len(dataArrays[dataIndex])):
                dataArrays[dataIndex][numberIndex] = float(dataArrays[dataIndex][numberIndex])
        # self.dataArrays = dataArrays
        self.stopHeight = float(dataArrays[17][2]) - float(dataArrays[5][0]) # Height from middle of outer object + height of middle of outer object
        self.stopLayer = math.floor((self.stopHeight - 0.1) / layerHeight)
        self.outerCenter = dataArrays[1]
        self.innerCenter = dataArrays[3]
        self.volume = dataArrays[7][0]
        self.contFile = dataArrays[9][0]
        self.modelName = newModelName
        self.modelObj = newModelObj
        self.adjustedSTLPath = scrappy_import_path + self.modelName[:-9] + '_' + self.name[:-9] + '.stl'
        self.adjustedScrappedSTLPath = scrappy_import_path + self.modelName[:-9] + '_' + self.name[:-9] + '_ScrappedVolume.stl'
        self.adjustedModelSTLPath = scrappy_import_path + self.modelName[:-9] + '_' + self.name[:-9] + '_EmptyModel.stl'
        if(self.lastValidRevision != revision):
            # new revision, so mesh isn't ready yet
            self.modelAvailable = False
        if (validity):
            self.lastValidRevision = revision
            self.lastValidTime = addTime
            self.lastValidMeshTime = meshTime
            self.scrapAvailable = True
        self.isValid = validity
        self.retryCount = 0
        # scrapRotationData = dataArrays[1][9]
        # scrapTranslationData = dataArrays[1][7]
        # scrapMatrixData = dataArrays[1][14]
        # scrapMatrixData.extend(dataArrays[1][15])
        # scrapMatrixData.extend(dataArrays[1][16])
        # scrapMatrixData.extend(dataArrays[1][17])
    def setMeshTime(self, meshTime):
        if (self.isValid):
            self.lastValidMeshTime = meshTime

    def setMesh(self, revision, addTime, meshTime):
        if (not self.isValid):
            raise Exception("Tried to set mesh from invalid scrap inclusion")
        if (self.lastValidRevision != revision):
            raise Exception("Tried to set wrong revision mesh for scrap. ScrapRevision: " + str(self.lastValidRevision) + ", ModelmeshRevision: " + str(revision))
        if (self.lastValidMeshTime != meshTime):
            raise Exception("Tried to set mesh for scrap with wrong meshTime. ScrapTime: " + str(self.lastValidMeshTime) + ", MeshTime: " + str(meshTime))
        if (self.lastValidTime != addTime):
            raise Exception("Tried to set mesh for scrap with wrong addTime. ScrapTime: " + str(self.lastValidTime) + ", AddTime: " + str(addTime))
        self.modelAvailable = True

    def generateHTML(self):
        returnString = '<div id="scrapObjDiv_' + str(self.name) + '" ' + generateStyleString(self.isValid)
        returnString += '    <img src="' + self.imagePath + '" style="width: 88vw;" alt="' + self.name[:-4] + '">\n'
        returnString += '    <div style="text-align: center;">'
        returnString += '<button type="scrapObjButton" onclick="sendSimpleSignal(\'PreviewScrap\', ' + str(self.ID) + ')">Preview ' + self.name[:-4] + '</button>'
        returnString += '<button type="scrapObjButton" onclick="sendSimpleSignal(\'RequestScrap\', ' + str(self.ID) + ')">Request mesh ' + self.name[:-4] + '</button>'
        if (self.modelAvailable):
            returnString += '<button type="scrapObjButton" onclick="sendSimpleSignal(\'IntegrateScrap\', ' + str(self.ID) + ')">Integrate ' + self.name[:-4] + '</button>'
        returnString += '</div>\n</div>\n'
        return returnString

    def increaseRetryCount(self):
        self.retryCount = self.retryCount + 1
        return self.retryCount

class queueItemData():
    def __init__(self, operation, scrapItem, revision, addTime, meshTime, modelObj = {}, useFileName = "", useFileFolder = "", misc = {}):
        self.operation = operation
        self.scrapItem = scrapItem
        self.revision = revision
        self.addTime = addTime
        self.meshTime = meshTime
        self.modelObj = modelObj
        self.fileToUse = useFileName
        self.fileToUseFolder = useFileFolder
        self.misc = misc

class timelineListenerThread(threading.Thread):
    def __init__(self, threadID, name):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.name = name
    def run(self):
        print ("Starting " + self.name)
        backgroundTimelineListener(self.name)
        print ("Exiting " + self.name)

class matryoshkaThread(threading.Thread):
    def __init__(self, threadID, name, q):
        threading.Thread.__init__(self)
        self.threadID = threadID
        self.name = name
        self.q = q
    def run(self):
        print ("Starting " + self.name)
        backgroundRunner(self.name, self.q)
        print ("Exiting " + self.name)

def createSimplifiedSTL(maxPolygonCount, stlPath, simplePath, stlName, simplificationMode):
    script_dir = os.path.dirname(__file__) #<-- absolute dir the script is in
    rel_path_blend = "Resources/Hidden/BGScript.blend"
    rel_path_script = "Resources/Hidden/simplify.py"
    
    if (simplificationMode):
        rel_path_script = "Resources/Hidden/blobbify.py"

    abs_path_blend = os.path.join(script_dir, rel_path_blend)
    abs_path_script = os.path.join(script_dir, rel_path_script)
    abs_path_blend = abs_path_blend.replace("\\", "/")
    abs_path_script = abs_path_script.replace("\\", "/")

    # Run Blender with one of the scripts to simplify the model
    p = subprocess.Popen("blender " + abs_path_blend + " --background --python " + abs_path_script + " -- " + str(maxPolygonCount) + " " + simplePath + " " + stlPath + " " + stlName + "", shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    # stdout = p.communicate()[0]
    # print('STDOUT:{}'.format(stdout))
    return_value = p.wait()
    print("Simplification return value: " + str(return_value) + " for: " + stlName + " Mode: " + str(simplificationMode))

    if not (os.path.isfile(simplePath)):
        raise Exception("Simplifying mesh failed. Path: " + stlPath + "; Target: " + simplePath) 

def backgroundRunner(threadName, q):
    while not exitFlag:
        if (queueLock.acquire(True, 0.1)): # Actively try to acquire lock for 0.1 seconds
            if not workQueue.empty():
                data = q.get()[3] # first three are just for ordering
                queueLock.release()
                print("%s processing %s on %s" % (threadName, data.operation, data.scrapItem.originalName))
                if(data.operation == "posCheck"):
                    checkScrapPosition(data.scrapItem, data.revision, data.addTime)
                elif(data.operation == "getMesh"):
                    getMatryoshkaMeshes(data.scrapItem, data.revision, data.addTime, data.meshTime)
                elif(data.operation == "sendFile"):
                    threadSendFile(data.scrapItem, data.modelObj, data.fileToUse, data.fileToUseFolder)
                elif(data.operation == "simplify"):
                    threadSimplify(data.scrapItem, data.modelObj, data.misc, data.fileToUseFolder)
            else:
                queueLock.release()
                # print(threadName + " is sleeping")
                time.sleep(1)
                #  adsk.doEvents()
        else:
            print(threadName + " was unable to acquire lock.")
            time.sleep(1)

def backgroundTimelineListener(threadName):
    global lastMakerPosition
    app = adsk.core.Application.get()
    design = adsk.fusion.Design.cast(app.activeProduct)
    while not exitFlag:
        if design.designType == 1:
            if(gTimeline.markerPosition != lastMakerPosition and gTimeline.markerPosition > 0):
                lastMakerPosition = gTimeline.markerPosition
                print("Listener: Updating all scrap")
                print("***************************************************************************")
                updateTime = datetime.datetime.now()
                updateAllScrap(updateTime)
            time.sleep(0.2)
        else:
            time.sleep(1)

def updateAllScrap(updateTime):
    # If we update, it means we are at a new revision (or started by user).
    print("Updating all scrap_in function " + str(updateTime))
    revisionLock.acquire()
    global revisionCounter
    global scrapList
    revisionCounter += 1

    

    for scrapItem in scrapList:
        scrapItem.isValid = False
        scrapItem.modelAvailable = False
        scrapItem.scrapAvailable = False

    initializeHTML() # reset HTML for all scrap

    app = adsk.core.Application.get()
    design = adsk.fusion.Design.cast(app.activeProduct)
    rootComp = design.rootComponent

    # Specify the folder to write out the results.
    base_path = scrappy_export_path
    filename = str(revisionCounter) + '_Full.stl'
    simple_name = filename[:-9] + "_Simp.stl"
    blob_name = filename[:-9] + "_Blob.stl"
    file_path = base_path + filename
    simple_path = base_path + simple_name
    blob_path = base_path + blob_name
    file_path = file_path.replace("\\", "/")
    simple_path = simple_path.replace("\\", "/")
    blob_path = blob_path.replace("\\", "/")
    
    # Save the file as STL.
    exportMgr = adsk.fusion.ExportManager.cast(design.exportManager)
    stlOptions = exportMgr.createSTLExportOptions(rootComp)
    stlOptions.meshRefinement = adsk.fusion.MeshRefinementSettings.MeshRefinementMedium
    stlOptions.filename = file_path
    exportMgr.execute(stlOptions)

    # Create model object
    modelObj = modelObject(revisionCounter, filename, file_path)
    modelList.append(modelObj)

    # Parallel simplify: blobbify (bounding box) and send it to the server for later
    simplifyDetails = type('obj', (object,), {'type':'Model', 'mode':'Blob', 'send':True})() # Also send the blob file to server
    queueLock.acquire()
    # remove old scrap checks for the later operations already
    workQueue.queue.clear()
    addTime = datetime.datetime.now()
    newQueueItemDataBlob = queueItemData("simplify", scrapList[0], revisionCounter, addTime, -1, modelObj, '', 'ExportedDesigns', simplifyDetails) # random scrapItem
    workQueue.put((49, -scrapList[0].volume, next(unique), newQueueItemDataBlob)) # blobbify file needed later in parallel
    queueLock.release()

    # Simplify STL immediately since we need it now
    createSimplifiedSTL(1750, file_path, simple_path, filename[:-4], 0)
    modelObj.simplePath = simple_path

    # Send current simple model STL to server # Send immediately
    scrappyClient.start_client('clientOperations.sendSTL', {}, modelObj, simple_name, 'ExportedDesigns') 

    # send files needed later parallel in threads # TODO: A bit dubious without check, this should be separate threads that are synchronized before mesh generation.
    queueLock.acquire()
    # Send full model version as well
    # newQueueItemData = queueItemData("sendFile", scrapList[0], revisionCounter, addTime, -1, modelObj, blob_name, 'ExportedDesigns') # random scrapItem
    # workQueue.put((50, -scrapList[0].volume, next(unique), newQueueItemData)) # send files needed later in parallel
    newQueueItemData = queueItemData("sendFile", scrapList[0], revisionCounter, addTime, -1, modelObj, filename, 'ExportedDesigns') # random scrapItem
    workQueue.put((50, -scrapList[0].volume, next(unique), newQueueItemData)) # send files needed later in parallel
    queueLock.release()

    # Fill queue for scrap checks
    queueLock.acquire()
    operation = "posCheck"
    global totalVolume

    # print("Start:")
    for scrapItem in scrapList:
        # print("Adding scrap check to queue: " + scrapItem.originalName)
        newQueueItemData = queueItemData(operation, scrapItem, revisionCounter, addTime, -1)
        workQueue.put((300, -scrapItem.volume, next(unique), newQueueItemData)) # position checks
        totalVolume += scrapItem.volume
        # print(scrapItem.volume)
    queueLock.release()
    # print("Sorted:")
    if (totalVolume > 0):
        scrapList = sorted(scrapList, reverse = True)
    # for scrapItem in scrapList:
    #     print(scrapItem.volume)

    print(workQueue.qsize())
    revisionLock.release()

def threadSendFile(scrapItem, modelObj, fileName, fileFolder):
    print("Thread sending file: " + str(fileName))
    scrappyClient.start_client('clientOperations.sendSTL', scrapItem, modelObj, fileName, fileFolder) 

def threadSimplify(scrapItem, modelObj, detailsObject, targetFolder):
    if (detailsObject.type == "Scrap"):
        if (detailsObject.mode == "Simplify"):
            createSimplifiedSTL(1750, scrapItem.originalPath, scrapItem.simplePath, scrapItem.originalName[:-4], 0)
        elif (detailsObject.mode == "Blob"):
            raise Exception("Blobbify for scraps currently not supported")
    
    elif (detailsObject.type == "Model"):
        if (detailsObject.mode == "Simplify"):
            # Specify the folder to write out the results.
            base_path = scrappy_export_path
            filename = modelObj.name
            simple_name = filename[:-9] + "_Simp.stl"
            file_path = base_path + filename
            simple_path = base_path + simple_name
            file_path = file_path.replace("\\", "/")
            simple_path = simple_path.replace("\\", "/")
            
            # Simplify STL
            createSimplifiedSTL(1750, file_path, simple_path, filename[:-4], 0)
            modelObj.simplePath = simple_path

        elif (detailsObject.mode == "Blob"):
            # Specify the folder to write out the results.
            base_path = scrappy_export_path
            filename = modelObj.name
            blob_name = filename[:-9] + "_Blob.stl"
            file_path = base_path + filename
            blob_path = base_path + blob_name
            file_path = file_path.replace("\\", "/")
            blob_path = blob_path.replace("\\", "/")
            
            # Blobbify STL
            createSimplifiedSTL(1750, file_path, blob_path, filename[:-4], 1)
            modelObj.blobPath = blob_path

            if (detailsObject.send == True):
                # send files needed later parallel in threads # TODO: A bit dubious without check, this should be separate threads that are synchronized before mesh generation.
                queueLock.acquire()
                addTime = datetime.datetime.now()
                # Send other versions of model as well
                newQueueItemData = queueItemData("sendFile", scrapList[0], revisionCounter, addTime, -1, modelObj, blob_name, targetFolder) # random scrapItem
                workQueue.put((50, -scrapList[0].volume, next(unique), newQueueItemData)) # send files needed later in parallel
                queueLock.release()


def checkScrapPosition(scrapItem, revision, addTime):
    # Stuff to get the mesh into the workspace/modifying the CPS

    # app = adsk.core.Application.get()
    # design = adsk.fusion.Design.cast(app.activeProduct)
    # rootComp = design.rootComponent
    # ui  = app.userInterface
    # doc = app.activeDocument
    # products = doc.products
    # product = products.itemByProductType('CAMProductType')
    # cam = adsk.cam.CAM.cast(product)
    filename = str(revision) + '_Full.stl'
    modelObj = {}
    for modelO in modelList:
        if modelO.name == filename:
            modelObj = modelO
    # scrapFileName = scrapItem.name
    scrapItem.modelObj = modelObj
    adsk.doEvents()

    # Send full version of scrap for later (gets skipped after first time sending)
    queueLock.acquire()
    newQueueItemData = queueItemData("sendFile", scrapItem, revision, addTime, -1, modelObj, scrapItem.name, 'LibraryFull') 
    workQueue.put((50, -scrapList[0].volume, next(unique), newQueueItemData)) # send files needed later in parallel
    queueLock.release()


    returnData = scrappyClient.start_client('clientOperations.scrapTest', scrapItem, scrapItem.modelObj, scrapItem.simpleName, "LibrarySimplified")
    if (returnData[0] == "0"):
        # Get repositioned scrap
        # scrappyClient.start_client('clientOperations.receiveScrap', scrapItem, filename, scrapFileName)
        getMeshTime = datetime.datetime.now()
        scrapItem.updateDetails(returnData[1], filename, modelObj, revision, addTime, getMeshTime, True)
        print(scrapItem.simpleName + " fits. Exit code: " + returnData[0])

        if (autoGenerate):
            queueLock.acquire()
            newQueueItemData = queueItemData("getMesh", scrapItem, revision, addTime, getMeshTime)
            workQueue.put((400, -scrapItem.volume, next(unique), newQueueItemData)) # add getting this mesh to the work queue
            queueLock.release()
    else: # Scrap was determined to not fit
        scrapItem.isValid = False
        scrapItem.modelAvailable = False
        scrapItem.scrapAvailable = False
        print(scrapItem.simpleName + " did not fit. Exit code: " + returnData[0] + ", #data received: " + str(len(returnData[1])))
    # Either way, update library panel
    updateLibraryPanel(scrapItem)
    # initializeHTML()
    # stopLayers = [stopLayer]

def getMatryoshkaMeshes(scrapItem, revision, addTime, meshTime):
    filename = str(revision) + '_Full.stl'
    blob_name = str(revision) + '_Blob.stl'
    # scrapFileName = scrapItem.name
    # scrapSimpleName = scrapItem.simpleName

    adsk.doEvents()

    if (scrapItem.retryCount >= 1):
        time.sleep(0.1) # give time to close previous runs connections

    # get mesh with scrap removed
    returnData = scrappyClient.start_client('clientOperations.meshGeneration', scrapItem, scrapItem.modelObj)

    if (returnData[0] == "0"):
        if (not scrapItem.isValid or scrapItem.lastValidRevision != revision or scrapItem.lastValidTime != addTime or scrapItem.lastValidMeshTime != meshTime):
            print("Generated a mesh for a scrapItem inclusion that is (no longer or not at all) valid.")
            print("scrapItem.isValid:" + str(scrapItem.isValid))
            print("scrapItem.lastValidRevision:" + str(scrapItem.lastValidRevision))
            print("revision:" + str(revision))
            print("scrapItem.lastValidTime:" + str(scrapItem.lastValidTime))
            print("addTime:" + str(addTime))
            print("scrapItem.lastValidMeshTime:" + str(scrapItem.lastValidMeshTime))
            print("meshTime:" + str(meshTime))
        else:
            # get volume removed by scrap including insertion path
            scrappyClient.start_client('clientOperations.getScrappedVolume', scrapItem, scrapItem.modelObj)

            # get moved(translated) original mesh
            scrappyClient.start_client('clientOperations.receiveModel', scrapItem, scrapItem.modelObj)

            scrapItem.setMesh(revision, addTime, meshTime)

        updateLibraryPanel(scrapItem)

    else:
        retryCount = scrapItem.increaseRetryCount()
        print("Retrying: " + scrapItem.simpleName + " Try: " + str(retryCount))
        print("Return code: " + str(returnData[0]))
        print(returnData)

        if (retryCount <= 2):
            queueLock.acquire()
            newQueueItemData = queueItemData("getMesh", scrapItem, revision, addTime, meshTime)
            workQueue.put((450, -scrapItem.volume, next(unique), newQueueItemData)) # add getting this mesh to the work queue
            queueLock.release()

def updateLibraryPanel(scrapItem):
    global totalVolume
    global scrapList
    vol = 0
    for scrap in scrapList:
        vol += scrap.volume
    
    # Check if order of scrap has to be adjusted as well
    if (vol != totalVolume):
        scrapList = sorted(scrapList, reverse = True)
        totalVolume = vol
        initializeHTML()
    else:
        script_dir = os.path.dirname(__file__) #<-- absolute dir the script is in
        HTML_file_path = os.path.join(script_dir, "scrapLibrary.html")
        New_HTML_file_path = os.path.join(script_dir, "scrapLibrary.html")

        divDelimiter = '<!--' + scrapItem.originalName + '-->'
        newScrapDiv = divDelimiter + scrapItem.generateHTML() # We keep the one at the end

        HTMLLock.acquire()
        inFile = open(HTML_file_path, "r")
        inString = inFile.read()
        inFile.close()
        startPos = inString.index(divDelimiter)
        inString = inString.replace(divDelimiter, "", 1) # remove the first one, search again
        endPos = inString.index(divDelimiter)
        replacePart = inString[startPos:endPos]
        inString = inString.replace(replacePart, newScrapDiv, 1) # replace div element for scrapItem
        outFile = open(New_HTML_file_path, "w")
        outFile.write(inString)
        outFile.close()

        # # Update palette with new HTML
        # try:
        #     palette = _ui.palettes.itemById('scrapLibraryPalette')
        #     palette.htmlFileURL = New_HTML_file_path
        # except:
        #     print("PALETTE COULD NOT BE FOUND!")

        HTMLLock.release()


def initializeHTML():
    print("initializeHTML()")
    script_dir = os.path.dirname(__file__) #<-- absolute dir the script is in
    scrapObjDivsHTML = ''
    rel_path_scrap = "Resources/Objects"
    abs_directory_path_scrap = os.path.join(script_dir, rel_path_scrap)
    for scrapItem in scrapList:
        divDelimiter = '<!--' + scrapItem.originalName + '-->' # Unique, as they are all within the same folder
        scrapObjDivsHTML += divDelimiter + scrapItem.generateHTML() + divDelimiter
    
    rel_path_in = "Resources/Hidden/scrapLibraryBASE.html"
    rel_path_out = "scrapLibrary.html"
    abs_file_path_in = os.path.join(script_dir, rel_path_in)
    abs_file_path_out = os.path.join(script_dir, rel_path_out)
    HTMLLock.acquire()
    inFile = open(abs_file_path_in, "r")
    inString = inFile.read().replace('<div id="addScrapObjDivsDiv"></div>', scrapObjDivsHTML, 1)
    # counterStringReplacement = '<p id="uniqueCounter">' + str(unique) + '</p>'
    # inString = inString.replace('<p id="uniqueCounter">0</p>', counterStringReplacement, 1)
    outFile = open(abs_file_path_out, "w")
    outFile.write(inString)
    inFile.close()
    outFile.close()
    
    # # Update palette with new HTML
    # try:
    #     palette = ui.palettes.itemById('scrapLibraryPalette')
    #     palette.htmlFileURL = 'scrapLibrary.html'
    # except:
    #     print("PALETTE COULD NOT BE FOUND!")

    # adsk.doEvents()

    HTMLLock.release()


def updateHTML():
    # Update palette with new HTML
    HTMLLock.acquire()
    try:
        palette = _ui.palettes.itemById('scrapLibraryPalette')
        palette.htmlFileURL = 'scrapLibrary.html'
    except:
        print("PALETTE COULD NOT BE FOUND!")
    HTMLLock.release()
    adsk.doEvents()

def generateStyleString(visibility):
    displayString = 'None;'
    if (visibility):
        displayString = 'Block;'
    return 'style="display: ' + displayString + ' width: 88vw; margin-top: 5px; margin-bottom: 5px; text-align: left;">\n'

def requestMesh(scrapItem):
    global revisionCounter
    getMeshTime = datetime.datetime.now()
    scrapItem.setMeshTime(getMeshTime)
    queueLock.acquire()
    newQueueItemData = queueItemData("getMesh", scrapItem, revisionCounter, scrapItem.lastValidTime, getMeshTime)
    workQueue.put((200, -scrapItem.volume, next(unique), newQueueItemData)) # add getting this mesh to the work queue
    queueLock.release()

def previewScrap(scrapItem):
    # Prepare materials
    if (scrapItem.isValid and scrapItem.scrapAvailable):
        app = adsk.core.Application.get()
        lib = app.materialLibraries.itemByName('Fusion 360 Material Library')
        design = adsk.fusion.Design.cast(app.activeProduct)
        root = design.rootComponent
        transparentMaterial = lib.materials.itemByName('Polycarbonate, Clear') #Plastic - Matte (Yellow)
        redShinyMaterial = lib.materials.itemByName('Aluminum, Anodized Red')

        # Model
        # Set design type to 1 for adding meshes
        # design.designType = 1
        if design.designType == adsk.fusion.DesignTypes.DirectDesignType:
            design.designType = adsk.fusion.DesignTypes.ParametricDesignType
        
        modelCollection = adsk.core.ObjectCollection.create()
        print("Number of bodies: " + str(root.meshBodies.count))
        # l = help(root)
        for j in range(0, root.meshBodies.count):
            body = root.meshBodies.item(j)
            body.material = transparentMaterial # set material
            body.opacity = 0.4
            # root.opacity = 0.2
            adsk.doEvents()
            
            modelCollection.add(body)
            # adsk.doEvents()
    

        # Set design type to 0 for manipulating meshes
        if design.designType == adsk.fusion.DesignTypes.ParametricDesignType:
            design.designType = adsk.fusion.DesignTypes.DirectDesignType

        # Model: undoing the centering
        modelCenterTranslationMatrix = adsk.core.Matrix3D.create()
        modelCenterTranslation = adsk.core.Vector3D.create(scrapItem.outerCenter[0], scrapItem.outerCenter[1], scrapItem.outerCenter[2])
        modelCenterTranslation.scaleBy(0.1)
        modelCenterTranslationMatrix.translation = modelCenterTranslation

        # Applying the model transformations
        # modelFeatures = root.features.moveFeatures
        # if (modelCenterTranslationMatrix.translation.length > 0.00001):
        #     modelFeatureInputCenT = modelFeatures.createInput(modelCollection, modelCenterTranslationMatrix)
        #     modelFeatures.add(modelFeatureInputCenT)

        # New Scrap
        # Set design type to 1 for adding meshes
        design.designType = 1

        scrapMeshList = design.rootComponent.meshBodies.add(scrapItem.adjustedSTLPath, adsk.fusion.MeshUnits.MillimeterMeshUnit)
        scrapMesh = scrapMeshList.item(0)
        scrapMesh.material = redShinyMaterial # set material

        scrapCollection = adsk.core.ObjectCollection.create()
        scrapCollection.add(scrapMesh)

        # Set design type to 0 for manipulating meshes
        design.designType = adsk.fusion.DesignTypes.DirectDesignType
        # New Scrap: undoing the centering
        scrapCenterTranslationMatrix = adsk.core.Matrix3D.create()
        scrapCenterTranslation = adsk.core.Vector3D.create(scrapItem.innerCenter[0], scrapItem.innerCenter[1], scrapItem.innerCenter[2])
        scrapCenterTranslation.scaleBy(0.1)
        scrapCenterTranslationMatrix.translation = scrapCenterTranslation
        # print("scrapCenterTranslationMatrix")
        # print(scrapCenterTranslationMatrix.asArray())

        # Applying the scrap transformations
        scrapFeatures = root.features.moveFeatures    

        if (modelCenterTranslationMatrix.translation.length > 0.00001):
            scrapFeatureInputModelCenT = scrapFeatures.createInput(scrapCollection, modelCenterTranslationMatrix)
            scrapFeatures.add(scrapFeatureInputModelCenT)

        if (scrapCenterTranslationMatrix.translation.length > 0.00001):
            scrapFeatureInputCenT = scrapFeatures.createInput(scrapCollection, scrapCenterTranslationMatrix)
            scrapFeatures.add(scrapFeatureInputCenT)
            print("Centering translation done:")
            print(scrapCenterTranslationMatrix.asArray())
        else:
            print("Centering translation too small")

    else:
        print("SCRAP NOT VALID OR AVAILABLE!")
        print("Valid: " + str(scrapItem.isValid))
        print("Available: " + str(scrapItem.scrapAvailable))

def integrateScrap(scrapItem):
    print("Integrating scrap! " + scrapItem.simpleName)
    # Is scrap ready to be integrated?
    if (scrapItem.isValid and scrapItem.modelAvailable and scrapItem.scrapAvailable):
        print(scrapItem.adjustedModelSTLPath)
        # Prepare materials
        app = adsk.core.Application.get()
        lib = app.materialLibraries.itemByName('Fusion 360 Material Library')
        design = adsk.fusion.Design.cast(app.activeProduct)
        root = design.rootComponent
        transparentMaterial = lib.materials.itemByName('Polycarbonate, Clear') #Plastic - Matte (Yellow)
        redShinyMaterial = lib.materials.itemByName('Aluminum, Anodized Red')

        # First, hide existing bodies
        for j in range(0, root.meshBodies.count):
            root.meshBodies.item(j).isLightBulbOn = False 
            adsk.doEvents()

        # Model
        # Set design type to 1 for adding meshes
        # design.designType = 1
        if design.designType == adsk.fusion.DesignTypes.DirectDesignType:
            design.designType = adsk.fusion.DesignTypes.ParametricDesignType
        
        modelCollection = adsk.core.ObjectCollection.create()

        modelMeshList = design.rootComponent.meshBodies.add(scrapItem.adjustedModelSTLPath, adsk.fusion.MeshUnits.MillimeterMeshUnit) # CentimeterMeshUnit
        modelMesh = modelMeshList.item(0)
        modelMesh.material = transparentMaterial # set material
        modelMesh.opacity = 0.4
        modelCollection.add(modelMesh)

        # Set design type to 0 for manipulating meshes
        if design.designType == adsk.fusion.DesignTypes.ParametricDesignType:
            design.designType = adsk.fusion.DesignTypes.DirectDesignType

        # Model: undoing the centering
        modelCenterTranslationMatrix = adsk.core.Matrix3D.create()
        modelCenterTranslation = adsk.core.Vector3D.create(scrapItem.outerCenter[0], scrapItem.outerCenter[1], scrapItem.outerCenter[2])
        modelCenterTranslation.scaleBy(0.1)
        modelCenterTranslationMatrix.translation = modelCenterTranslation

        # Applying the model transformations
        modelFeatures = root.features.moveFeatures
        if (modelCenterTranslationMatrix.translation.length > 0.00001):
            modelFeatureInputCenT = modelFeatures.createInput(modelCollection, modelCenterTranslationMatrix)
            modelFeatures.add(modelFeatureInputCenT)

        # New Scrap
        # Set design type to 1 for adding meshes
        if design.designType == adsk.fusion.DesignTypes.DirectDesignType:
            design.designType = adsk.fusion.DesignTypes.ParametricDesignType
        scrapMeshList = design.rootComponent.meshBodies.add(scrapItem.adjustedSTLPath, adsk.fusion.MeshUnits.MillimeterMeshUnit)
        scrapMesh = scrapMeshList.item(0)
        scrapMesh.material = redShinyMaterial # set material

        scrapCollection = adsk.core.ObjectCollection.create()
        scrapCollection.add(scrapMesh)

        # Set design type to 0 for manipulating meshes
        if design.designType == adsk.fusion.DesignTypes.ParametricDesignType:
            design.designType = adsk.fusion.DesignTypes.DirectDesignType
        
        # New Scrap: undoing the centering
        scrapCenterTranslationMatrix = adsk.core.Matrix3D.create()
        scrapCenterTranslation = adsk.core.Vector3D.create(scrapItem.innerCenter[0], scrapItem.innerCenter[1], scrapItem.innerCenter[2])
        scrapCenterTranslation.scaleBy(0.1)
        scrapCenterTranslationMatrix.translation = scrapCenterTranslation

        # Applying the scrap transformations
        scrapFeatures = root.features.moveFeatures    

        if (modelCenterTranslationMatrix.translation.length > 0.00001):
            scrapFeatureInputModelCenT = scrapFeatures.createInput(scrapCollection, modelCenterTranslationMatrix)
            scrapFeatures.add(scrapFeatureInputModelCenT)

        # if (scrapCenterTranslationMatrix.translation.length > 0.00001):
        #     scrapFeatureInputCenT = scrapFeatures.createInput(scrapCollection, scrapCenterTranslationMatrix)
        #     scrapFeatures.add(scrapFeatureInputCenT)
        #     print("Centering translation done:")
        #     print(scrapCenterTranslationMatrix.asArray())
        # else:
        #     print("Centering translation too small")

    else:
        print("SCRAP NOT VALID OR MESH UNAVAILABLE!")

def run(context):
    # Build UI additions to Fusion
    ui = None
    
    # Reset globals in case of restart of add-in
    global totalVolume
    global revisionCounter
    global stopLayers
    global scrapList
    totalVolume = 0
    revisionCounter = 0
    stopLayers = []
    scrapList = []
    
    try:
        app = adsk.core.Application.get()
        ui  = app.userInterface

        doc = app.activeDocument
        design = app.activeProduct
        global gTimeline
        # TODO: Also first change environment to Modeling/Design
        allWorkspaces = ui.workspaces
        designWorkspace = allWorkspaces.itemById('FusionSolidEnvironment')
        designWorkspace.activate()
        design.designType = 1
        try:
            gTimeline = design.timeline
        except:
            print("Timeline was not available!")
        products = doc.products
        product = products.itemByProductType('CAMProductType')
        cam = adsk.cam.CAM.cast(product)

        # Create scrap object list from folder
        script_dir = os.path.dirname(__file__) #<-- absolute dir the script is in
        rel_path_scrap = "Resources/Library"
        rel_path_scrap_image = "Resources/LibraryImages"
        rel_path_scrap_simple = "Resources/LibrarySimplified"
        rel_path_scrap_full = "Resources/LibraryFull"
        abs_directory_path_scrap = os.path.join(script_dir, rel_path_scrap)
        abs_directory_path_scrap_image = os.path.join(script_dir, rel_path_scrap_image)
        abs_directory_path_scrap_simple = os.path.join(script_dir, rel_path_scrap_simple)
        abs_directory_path_scrap_full = os.path.join(script_dir, rel_path_scrap_full)
        scrapCounter = 0
        for file in os.listdir(abs_directory_path_scrap):
            if (file.endswith(".stl")): #or file.endswith(".STL")): # obj, ...
                scrap_file_path = os.path.join(abs_directory_path_scrap, file)
                scrap_image_path = os.path.join(abs_directory_path_scrap_image, file)
                scrap_simple_path = os.path.join(abs_directory_path_scrap_simple, file)
                scrap_full_path = os.path.join(abs_directory_path_scrap_full, file)
                scrap_file_path = scrap_file_path.replace("\\", "/")
                scrap_image_path = scrap_image_path.replace("\\", "/")
                scrap_simple_path = scrap_simple_path.replace("\\", "/")
                scrap_full_path = scrap_full_path.replace("\\", "/")
                
                scrap_image_path = scrap_image_path[:-4] + ".png"
                scrap_full_path = scrap_full_path[:-4] + "_Full.stl"
                scrap_simple_path = scrap_simple_path[:-4] + "_Simp.stl"
                copiesAvailable = 1 # TODO: Allow the user to enter how many of those he has, remove them, add them to the folder, etc.
                newScrapObj = scrapObject(scrapCounter, scrap_file_path, scrap_image_path, scrap_simple_path, scrap_full_path, file, copiesAvailable)
                scrapList.append(newScrapObj)
                scrapCounter = scrapCounter + 1

        # For this example, we are adding the already exisiting 'Extrude' command into a new panel:
        cmdDefinitions = ui.commandDefinitions
        anotheraddInsCmd = cmdDefinitions.itemById('ScriptsManagerCommand')

        #ui.messageBox(properties.pauseLayerNumber)
        #ui.messageBox(postProperties)
        
        list = []
        script_dir = os.path.dirname(__file__) #<-- absolute dir the script is in
        rel_path = "Resources/TestOutput.txt"
        abs_file_path = os.path.join(script_dir, rel_path)
        file = open(abs_file_path, "w")

        # Lots of debug stuff to determine object hierarchy in Fusion360
        if(False):
            setup = cam.setups.item(0)
            operations = setup.allOperations

            rel_path_config = "Resources/prusa.cps"
            programName = 'GimmeTheDamnProperties'
            outputFolder = 'D:/TEMP'
            abs_file_path_config = os.path.join(script_dir, rel_path_config)
            postConfig = abs_file_path_config
            units = adsk.cam.PostOutputUnitOptions.DocumentUnitsOutput
            postInput = adsk.cam.PostProcessInput.create(programName, postConfig, outputFolder, units)
            postInput.isOpenInEditor = False
            r = cam.postProcess(setup, postInput)

            rel_path_config2 = "Resources/ultimaker2.cps"
            programName2 = 'GimmeTheDamnProperties2'
            abs_file_path_config2 = os.path.join(script_dir, rel_path_config2)
            postConfig2 = abs_file_path_config2
            postInput2 = adsk.cam.PostProcessInput.create(programName2, postConfig2, outputFolder, units)
            postInput2.isOpenInEditor = False
            r2 = cam.postProcess(setup, postInput2)

        
        if (False):
            #l = help(setup)
            #l = help(setup.machine)
            l = help(setup)
            l = help(setup.machine)
            for op in setup.operations:
                print(op.name)
            for op in setup.patterns:
                print('Pattern')
                print(op.name)
            for op in setup.allOperations:
                print(op.name)
            # for op in setup.children:
            #     print(op.name)
            # for op in setup.fixtures:
            #     print(op.name)
            # for op in setup.folders:
            #     print(op.name)
            for op in setup.models:
                print("Model")
                print(op.name)
            
            print(setup.operations)
            print("setup.operations")
            print(setup.patterns)
            print("setup.patterns")
            print(setup.children)
            print("setup.children")
            #print(setup.operationType)
            #print("setup.operationType")
            print(setup.allOperations)
            print("setup.allOperations")
            print(setup.machine.id)
            print(setup.machine.description)
            print(setup.machine.model)
            print(setup.machine.vendor)

        

        
        for stuff in products:
            #ui.messageBox(str(cmdDefinitions.itemById('Scripts and Add-Ins')))
            #ui.messageBox(str(cmdDefinitions.itemById('Add-Ins')))
            
            list.append(str(stuff))
        if (False):
            #l = dir(products)
            file.writelines("Products:\n")
            #list.append(l)
            file.writelines(str(list))
            file.writelines("\n")
            list = []
            
            
            l = dir(postInput.postConfiguration)
            file.writelines("postInput.postConfiguration Dir:\n")
            list.append(l)
            file.writelines(str(list))
            file.writelines("\n")
            list = []
            
            l2 = vars(postInput.postConfiguration)
            file.writelines("postInput.postConfiguration Vars:\n")
            list.append(l2)
            file.writelines(str(list))
            file.writelines("\n")
            list = []
            
            l = help(postInput.postConfiguration)
            file.writelines("postInput.postConfiguration Help:\n")
            list.append(l)
            file.writelines(str(list))
            file.writelines("\n")
            list = []
            
            l = dir(postInput)
            file.writelines("postInput Dir:\n")
            list.append(l)
            file.writelines(str(list))
            file.writelines("\n")
            list = []
            
            l2 = vars(postInput)
            file.writelines("postInput Vars:\n")
            list.append(l2)
            file.writelines(str(list))
            file.writelines("\n")
            list = []
            
            l = help(postInput)
            file.writelines("postInput Help:\n")
            list.append(l)
            file.writelines(str(list))
            file.writelines("\n")
            list = []
            
            
            l = dir(products)
            file.writelines("Dir Products:\n")
            list.append(l)
            file.writelines(str(list))
            file.writelines("\n")
            list = []
                
            l = dir(app)
            file.writelines("App Dir:\n")
            list.append(l)
            file.writelines(str(list))
            file.writelines("\n")
            list = []
            
            l2 = vars(app)
            file.writelines("App Vars:\n")
            list.append(l2)
            file.writelines(str(list))
            file.writelines("\n")
            list = []
            
            l = help(app)
            file.writelines("App Help:\n")
            list.append(l)
            file.writelines(str(list))
            file.writelines("\n")
            list = []
            
            l3 = dir(app.data)
            file.writelines("Data Dir:\n")
            list.append(l3)
            file.writelines(str(list))
            file.writelines("\n")
            list = []
            
            l3 = vars(app.data)
            file.writelines("Data Vars:\n")
            list.append(l3)
            file.writelines(str(list))
            file.writelines("\n")
            list = []
        
        file.close() #to change file access modes 
        #subprocess.call(["dir"])
        #subprocess.run(['C:\Python38\python', '..\..\print.py'])
        

        # For a few months, the customer might run either classic UI or tabbed toolbar UI.
        # Find out what is being used:
        runningTabbedToolbar = ui.isTabbedToolbarUI

        if (runningTabbedToolbar):
            # Get all workspaces:
            allWorkspaces = ui.workspaces
            # Get the Design workspace:
            designWorkspace = allWorkspaces.itemById('FusionSolidEnvironment')
            if (designWorkspace):
                # Get all the tabs for the Design workspaces:
                allDesignTabs = designWorkspace.toolbarTabs
                if ((allDesignTabs.count > 0)):
                    # Add a new tab to the Render and Design workspaces:
                    scrappyTab = allDesignTabs.add('ScrappyFabrication_DesignTab', 'Scrappy Fabrication')
                    if (scrappyTab):
                        # Get all of the toolbar panels for the Scrappy Fab tab:
                        allScrappyTabPanels = scrappyTab.toolbarPanels

                        # Create a button command definition.
                        buttonMatryoshka = cmdDefinitions.addButtonDefinition('buttonMatryoshkaDefIdPython', 
                                                                'CMD Matryoshka', 
                                                                'Runs Matryoshka from a new CMD')#,
                                                                #'./Resources/Sample')

                                                                
                        
                        # Connect to the command created event.
                        matryoshkaCommandCreated = matryoshkaCommandCreatedEventHandler()
                        buttonMatryoshka.commandCreated.add(matryoshkaCommandCreated)
                        handlers.append(matryoshkaCommandCreated)


                        # Has the panel been added already?
                        # You'll get an error if you try to add this more than once to the tab.

              
                        #Activate the Design workspace before activating the newly added Tab
                        designWorkspace.activate()
                                

                        scrappyFabPanel = None


                        scrappyFabPanel = allScrappyTabPanels.itemById('AddInShortcut_PanelId')
                        if scrappyFabPanel is None:
                            # We have not added the panel already.  Go ahead and add it.
                            scrappyFabPanel = allScrappyTabPanels.add('AddInShortcut_PanelId', 'Add-Ins')
                            

                        if scrappyFabPanel:
                            # We want this panel to be visible:
                            scrappyFabPanel.isVisible = True
                            # Access the controls that belong to the panel:
                            scrappyFabPanelControls = scrappyFabPanel.controls

                            # Do we already have this command in the controls?  
                            # You'll get an error if you try to add it more than once to the panel:
                            addInsCmdControl =  None
                            addInsCmdControl = scrappyFabPanelControls.itemById('ScriptsManagerCommand')
                            if addInsCmdControl is None:
                            
                            # Activate the newly added Tab in Design workspace before adding Command to the Panel
                                 if designWorkspace.isActive: 
                                    designTab = allDesignTabs.itemById('ScrappyFabrication_DesignTab')
                                    if not designTab.isActive :
                                        activationState = designTab.activate()
                                        if activationState :
                                            if anotheraddInsCmd:
                                                # Go ahead and add the command to the panel:
                                                addInsCmdControl = scrappyFabPanelControls.addCommand(anotheraddInsCmd)
                                                if addInsCmdControl:
                                                    addInsCmdControl.isVisible = True
                                                    addInsCmdControl.isPromoted = True
                                                    addInsCmdControl.isPromotedByDefault = True
                                            
                            else:
                                # If the command is already added to the Panel check if it is visible and display a message
                                if designWorkspace.isActive:
                                    designTab = allDesignTabs.itemById('ScrappyFabrication_DesignTab')
                                    if not designTab.isActive :
                                        activationState = designTab.activate()
                                        if activationState :
                                            if scrappyFabPanel.isVisible:
                                                ui.messageBox('Do you see Best Design Panel now?')     
                                            else:
                                                totalControlsInPanel = scrappyFabPanelControls.count
                                                if (totalControlsInPanel == 1):
                                                    if addInsCmdControl.isVisible:
                                                        ui.messageBox('Not visible control')

                        scrappyFabPanel = allScrappyTabPanels.itemById('Matryoshka_PanelId')
                        if scrappyFabPanel is None:
                            # We have not added the panel already.  Go ahead and add it.
                            scrappyFabPanel = allScrappyTabPanels.add('Matryoshka_PanelId', 'Matryoshka')
                            

                        if scrappyFabPanel:
                            # We want this panel to be visible:
                            scrappyFabPanel.isVisible = True
                            # Access the controls that belong to the panel:
                            scrappyFabPanelControls = scrappyFabPanel.controls
                            
                            # Add the button to the bottom of the panel.
                            buttonControl2 = scrappyFabPanelControls.addCommand(buttonMatryoshka)
                            if buttonControl2:
                                buttonControl2.isVisible = True
                                buttonControl2.isPromoted = True
                                buttonControl2.isPromotedByDefault = True
                        
                            # Add Library palette
                            try:
                                global _ui, _app
                                _app = adsk.core.Application.get()
                                _ui  = _app.userInterface
                                
                                # Add a command that displays the panel.
                                showScrapLibraryPaletteCmdDef = _ui.commandDefinitions.itemById('showScrapLibraryPalette')
                                if not showScrapLibraryPaletteCmdDef:
                                    showScrapLibraryPaletteCmdDef = _ui.commandDefinitions.addButtonDefinition('showScrapLibraryPalette', 'Show Scrap Library Palette', 'Show the Scrap Library Palette', '')

                                    # Connect to Command Created event.
                                    onCommandCreated = ShowPaletteCommandCreatedHandler()
                                    showScrapLibraryPaletteCmdDef.commandCreated.add(onCommandCreated)
                                    handlers.append(onCommandCreated)
                                
                                 
                                # Add a command under ADD-INS panel which sends information from Fusion to the palette's HTML.
                                sendInfoCmdDef = _ui.commandDefinitions.itemById('sendInfoToScrapLibraryHTML')
                                if not sendInfoCmdDef:
                                    sendInfoCmdDef = _ui.commandDefinitions.addButtonDefinition('sendInfoToScrapLibraryHTML', 'Send info to Palette', 'Send Info to Palette HTML', '')

                                    # Connect to Command Created event.
                                    onCommandCreated = SendInfoCommandCreatedHandler()
                                    sendInfoCmdDef.commandCreated.add(onCommandCreated)
                                    handlers.append(onCommandCreated)

                                # Add the command to the toolbar.
                                ###panel = _ui.allToolbarPanels.itemById('SolidScriptsAddinsPanel')
                                cntrl = scrappyFabPanel.controls.itemById('showScrapLibraryPalette')
                                if not cntrl:
                                    scrappyFabPanel.controls.addCommand(showScrapLibraryPaletteCmdDef)

                                cntrl2 = scrappyFabPanel.controls.itemById('sendInfoToScrapLibraryHTML')
                                if not cntrl2:
                                    scrappyFabPanel.controls.addCommand(sendInfoCmdDef)
                            except:
                                if _ui:
                                    _ui.messageBox('Failed:\n{}'.format(traceback.format_exc()))
                        
                        #
                        # Only for reference: How to add new button to existing Addins Panel:
                        #

                        # Get the ADD-INS panel in the model workspace. 
                        addInsPanel = ui.allToolbarPanels.itemById('SolidScriptsAddinsPanel')
                        
                        # Add the button to the bottom of the panel.
                        buttonControl = addInsPanel.controls.addCommand(buttonMatryoshka)

        # Create new threads when add-in is loaded
        threadID = 1
        # tllThread = timelineListenerThread(threadID, "Timeline Listener Thread")
        # tllThread.start()
        # threads.append(tllThread)
        for tName in threadList:
            threadID += 1
            thread = matryoshkaThread(threadID, tName, workQueue)
            thread.start()
            threads.append(thread)

    except:
        if ui:
            ui.messageBox('Adding Tab and Buttons::Failed run():\n{}'.format(traceback.format_exc()))

    
    

def updateCPS(stopLayerList):
    # Update CPS G-code translation file
    script_dir = os.path.dirname(__file__) #<-- absolute dir the script is in
    rel_path_in = "Resources/Hidden/prusaBASE.cps"
    rel_path_out = "Resources/prusa.cps"
    abs_file_path_in = os.path.join(script_dir, rel_path_in)
    abs_file_path_out = os.path.join(script_dir, rel_path_out)
    inFile = open(abs_file_path_in, "r")
    stopLayerListString = "["
    for stopLayer in range(len(stopLayerList)):
        if(stopLayer > 0):
            stopLayerListString += ", "
        stopLayerListString += str(stopLayerList[stopLayer])
    stopLayerListString += "]"
    inString = inFile.read().replace("[987654321]", stopLayerListString, 1)
    outFile = open(abs_file_path_out, "w")
    outFile.write(inString)
    inFile.close()
    outFile.close()

# Event handler for the commandCreated event.
class matryoshkaCommandCreatedEventHandler(adsk.core.CommandCreatedEventHandler):
    def __init__(self):
        super().__init__()
    def notify(self, args):
        eventArgs = adsk.core.CommandCreatedEventArgs.cast(args)
        cmd = eventArgs.command

        # Connect to the execute event.
        onExecute = MatryoshkaCMDExecuteHandler()
        cmd.execute.add(onExecute)
        handlers.append(onExecute)


# Event handler for the execute event.
class MatryoshkaCMDExecuteHandler(adsk.core.CommandEventHandler):
    def __init__(self):
        super().__init__()
    def notify(self, args):
        eventArgs = adsk.core.CommandEventArgs.cast(args)

        print("Matryoshka_CMD")

        # Code to react to the event.
        app = adsk.core.Application.get()
        ui  = app.userInterface

        doc = app.activeDocument
        products = doc.products
        product = products.itemByProductType('CAMProductType')
        cam = adsk.cam.CAM.cast(product)
        
        returnData = []
        
        if (True):
            try:
                design = adsk.fusion.Design.cast(app.activeProduct)
                
                # Get the root component of the active design
                rootComp = design.rootComponent

                # Specify the folder to write out the results.
                base_path = scrappy_export_path

                # Let the view have a chance to paint just so you can watch the progress.
                adsk.doEvents()
                        
                # Construct the output filename.
                
                
                filename = str(revisionCounter) + '.stl'
                file_path = base_path + filename
                
                # Save the file as STL.
                exportMgr = adsk.fusion.ExportManager.cast(design.exportManager)
                stlOptions = exportMgr.createSTLExportOptions(rootComp)
                stlOptions.meshRefinement = adsk.fusion.MeshRefinementSettings.MeshRefinementMedium
                stlOptions.filename = file_path
                exportMgr.execute(stlOptions)


                adsk.doEvents()
                #_ui.messageBox(scrappyClient.target_host)
                scrappyClient.start_client('clientOperations.sendSTL', {}, filename, '')
                # scrapFileName = "EggR.stl"
                scrapFileName = "xyzCalibration.stl"
                # returnData = scrappyClient.start_client('clientOperations.scrapTest', filename, "xyzCalibration.stl")
                returnData = scrappyClient.start_client('clientOperations.testing', {}, filename, scrapFileName)
                stopLayer = 987654321
                if (returnData[0] == "0"):
                    stopHeight = float(returnData[1][17][2])
                    stopHeightAddon = float(returnData[1][5][0])
                    stopHeight -= stopHeightAddon
                    print(stopHeight)
                    stopLayer = math.floor((stopHeight - 0.1) / layerHeight)
                    print("StopLayer: " + str(stopLayer))
                stopLayers = [stopLayer]
                
                # # Update CPS G-code translation file
                updateCPS(stopLayers)

                '''
                for i in range(1,2):
                    for j in range(1,2):
                        length = i * 10
                        width = j * 10
                        #lengthParam.expression = str(length)
                        #widthParam.expression = str(width)
                        
                        # Let the view have a chance to paint just so you can watch the progress.
                        adsk.doEvents()
                        
                        # Construct the output filename.
                        filename = base_path + str(length) + 'x' + str(width) + '.stl'
                        
                        # Save the file as STL.
                        exportMgr = adsk.fusion.ExportManager.cast(design.exportManager)
                        stlOptions = exportMgr.createSTLExportOptions(rootComp)
                        stlOptions.meshRefinement = adsk.fusion.MeshRefinementSettings.MeshRefinementMedium
                        stlOptions.filename = filename
                        exportMgr.execute(stlOptions)
                '''

            except:
                if ui:
                    ui.messageBox('Export failed:\n{}'.format(traceback.format_exc()))

            # ui = None
            try:
                # design.designType = adsk.fusion.DesignTypes.DirectDesignType
                root = design.rootComponent

                if (returnData[0] == "0"):


                    for dataIndex in [1, 3, 5, 7, 11, 13, 15, 17, 18, 19, 20, 21]: #range(1, len(returnData[1]), 2):
                        # print(returnData[1][dataIndex])
                        for numberIndex in range(0, len(returnData[1][dataIndex])):
                            returnData[1][dataIndex][numberIndex] = float(returnData[1][dataIndex][numberIndex])
                        #     if (returnData[1][dataIndex][numberIndex] < 0.0001):
                        #         returnData[1][dataIndex][numberIndex] = 0
                        # print(returnData[1][dataIndex])

                    scrapRotationData = returnData[1][9]
                    scrapTranslationData = returnData[1][7]
                    scrapCenterTranslationData = returnData[1][3]
                    modelCenterTranslationData = returnData[1][1]
                    scrapMatrixData = returnData[1][14]
                    scrapMatrixData.extend(returnData[1][15])
                    scrapMatrixData.extend(returnData[1][16])
                    scrapMatrixData.extend(returnData[1][17])
                    
                    # for index in range(len(scrapRotationData)):
                    #     scrapRotationData[index] = scrapRotationData[index].replace("e", "E")
                    # for index in range(len(scrapTranslationData)):
                    #     scrapTranslationData[index] = scrapTranslationData[index].replace("e", "E")
                    # for index in range(len(scrapCenterTranslationData)):
                    #     print("%10.18f" % (float(scrapCenterTranslationData[index])))
                    #     scrapCenterTranslationData[index] = scrapCenterTranslationData[index].replace("e", "E")
                    #     print("%10.18f" % (float(scrapCenterTranslationData[index])))
                    # for index in range(len(modelCenterTranslationData)):
                    #     modelCenterTranslationData[index] = modelCenterTranslationData[index].replace("e", "E")
                    print("modelCenterTranslationData")
                    print(modelCenterTranslationData)
                    print("scrapCenterTranslationData")
                    print(scrapCenterTranslationData)
                    print("scrapTranslationData")
                    print(scrapTranslationData)
                    print("scrapRotationData")
                    print(scrapRotationData)
                    print("scrapMatrixData")
                    print(scrapMatrixData)
                

                newModelName = str(revisionCounter) + '-00-99.stl'
                newScrapName = str(revisionCounter) + '-01-0.stl'
                newModelPath = scrappy_import_path + newModelName
                print(newModelPath)

                scrapPath = scrappy_library_path + scrapFileName
                print(scrapPath)

                newScrapPath = scrappy_import_path + newScrapName
                print(newScrapPath)

                # Prepare materials
                lib = app.materialLibraries.itemByName('Fusion 360 Material Library')
                transparentMaterial = lib.materials.itemByName('Polycarbonate, Clear') #Plastic - Matte (Yellow)
                redShinyMaterial = lib.materials.itemByName('Aluminum, Anodized Red')

                # finalFilepath = 'path/to/Scrappy/Fusion360/ScrappyAdd-In/Resources/ImportedDesigns/1-00-99.stl'

                

                # Model
                # Set design type to 1 for adding meshes
                design.designType = 1
                print("Adding meshes design Type: " + str(design.designType))
                modelMeshList = design.rootComponent.meshBodies.add(newModelPath, adsk.fusion.MeshUnits.MillimeterMeshUnit) # CentimeterMeshUnit
                modelMesh = modelMeshList.item(0)
                adsk.doEvents()
                modelMesh.material = transparentMaterial # set material
                modelMesh.opacity = 0.4
                # root.opacity = 0.2
                adsk.doEvents()
                modelCollection = adsk.core.ObjectCollection.create()
                modelCollection.add(modelMesh)
                adsk.doEvents()

                

                # Set design type to 0 for manipulating meshes
                design.designType = adsk.fusion.DesignTypes.DirectDesignType
                print("Transformations design Type: " + str(design.designType))
                # Model: undoing the centering
                modelCenterTranslationMatrix = adsk.core.Matrix3D.create()
                modelCenterTranslation = adsk.core.Vector3D.create(modelCenterTranslationData[0], modelCenterTranslationData[1], modelCenterTranslationData[2])
                modelCenterTranslation.scaleBy(0.1)
                modelCenterTranslationMatrix.translation = modelCenterTranslation
                # print("modelCenterTranslationMatrix")
                # print(modelCenterTranslationMatrix.asArray())
                # Applying the model transformations
                moveFeatures = root.features.moveFeatures
                if (modelCenterTranslationMatrix.translation.length > 0.00001):
                    modelFeatureInputCenT = moveFeatures.createInput(modelCollection, modelCenterTranslationMatrix)
                    moveFeatures.add(modelFeatureInputCenT)

                # New Scrap
                # Set design type to 1 for adding meshes
                design.designType = 1
                print("Adding meshes design Type: " + str(design.designType))
                scrapMeshList = design.rootComponent.meshBodies.add(newScrapPath, adsk.fusion.MeshUnits.MillimeterMeshUnit)
                scrapMesh = scrapMeshList.item(0)
                scrapMesh.material = redShinyMaterial # set material

                scrapCollection = adsk.core.ObjectCollection.create()
                scrapCollection.add(scrapMesh)

                # Set design type to 0 for manipulating meshes
                design.designType = adsk.fusion.DesignTypes.DirectDesignType
                print("Transformations design Type: " + str(design.designType))
                # New Scrap: undoing the centering
                scrapCenterTranslationMatrix = adsk.core.Matrix3D.create()
                scrapCenterTranslation = adsk.core.Vector3D.create(scrapCenterTranslationData[0], scrapCenterTranslationData[1], scrapCenterTranslationData[2])
                scrapCenterTranslation.scaleBy(0.1)
                scrapCenterTranslationMatrix.translation = scrapCenterTranslation
                # print("scrapCenterTranslationMatrix")
                # print(scrapCenterTranslationMatrix.asArray())

                # Applying the scrap transformations
                scrapFeatures = root.features.moveFeatures    

                if (modelCenterTranslationMatrix.translation.length > 0.00001):
                    scrapFeatureInputModelCenT = scrapFeatures.createInput(scrapCollection, modelCenterTranslationMatrix)
                    scrapFeatures.add(scrapFeatureInputModelCenT)

                if (scrapCenterTranslationMatrix.translation.length > 0.00001):
                    # scrapFeatureInputCenT = scrapFeatures.createInput(scrapCollection, scrapCenterTranslationMatrix)
                    # scrapFeatures.add(scrapFeatureInputCenT)
                    print("Centering translation done:")
                    print(scrapCenterTranslationMatrix.asArray())
                else:
                    print("Centering translation too small")


                # Finally, turn off visibility of original model
                root.meshBodies.item(0).isLightBulbOn = False 


                '''
                # Scrap
                # Set design type to 1 for adding meshes
                design.designType = 1
                print("Adding meshes design Type: " + str(design.designType))
                scrapMeshList = design.rootComponent.meshBodies.add(scrapPath, adsk.fusion.MeshUnits.MillimeterMeshUnit)
                scrapMesh = scrapMeshList.item(0)
                scrapCollection = adsk.core.ObjectCollection.create()
                scrapCollection.add(scrapMesh)

                # Set design type to 0 for manipulating meshes
                design.designType = adsk.fusion.DesignTypes.DirectDesignType
                print("Transformations design Type: " + str(design.designType))
                # Scrap: undoing the centering
                scrapCenterTranslationMatrix = adsk.core.Matrix3D.create()
                scrapCenterTranslation = adsk.core.Vector3D.create(scrapCenterTranslationData[0], scrapCenterTranslationData[1], scrapCenterTranslationData[2])
                scrapCenterTranslation.scaleBy(0.1)
                scrapCenterTranslationMatrix.translation = scrapCenterTranslation
                print("scrapCenterTranslationMatrix")
                print(scrapCenterTranslationMatrix.asArray())

                # print(scrapRotationMatrix.asArray())
                # Scrap: "cen" translation
                scrapTranslationMatrix = adsk.core.Matrix3D.create()
                scrapTranslation = adsk.core.Vector3D.create(scrapTranslationData[0], scrapTranslationData[1], scrapTranslationData[2])
                scrapTranslation.scaleBy(0.1)
                scrapTranslationMatrix.translation = scrapTranslation
                # Scrap: "th" rotation
                scrapRotationMatrix = adsk.core.Matrix3D.create()
                scrapRotationMatrix.translation = scrapTranslation
                rotX = adsk.core.Matrix3D.create()
                rotX.setToRotation(scrapRotationData[0], adsk.core.Vector3D.create(1,0,0), adsk.core.Point3D.create(scrapTranslationData[0],scrapTranslationData[1],scrapTranslationData[2]))
                scrapRotationMatrix.transformBy(rotX)
                rotY = adsk.core.Matrix3D.create()
                rotY.setToRotation(scrapRotationData[1], adsk.core.Vector3D.create(0,1,0), adsk.core.Point3D.create(scrapTranslationData[0],scrapTranslationData[1],scrapTranslationData[2]))
                scrapRotationMatrix.transformBy(rotY)
                rotZ = adsk.core.Matrix3D.create()
                rotZ.setToRotation(scrapRotationData[2], adsk.core.Vector3D.create(0,0,1), adsk.core.Point3D.create(scrapTranslationData[0],scrapTranslationData[1],scrapTranslationData[2]))
                scrapRotationMatrix.transformBy(rotZ)

                scrapRotationMatrix2 = adsk.core.Matrix3D.create()
                scrapRotationMatrix2.setWithArray(scrapMatrixData)
                scrapRotationMatrix2.translation = scrapCenterTranslation
                print("scrapRotationMatrix")
                print(scrapRotationMatrix.asArray())
                print("scrapRotationMatrix2")
                print(scrapRotationMatrix2.asArray())


                # Applying the scrap transformations
                scrapFeatures = root.features.moveFeatures    

                # scrapFeatureInputRot = scrapFeatures.createInput(scrapCollection, scrapRotationMatrix)
                # scrapFeatures.add(scrapFeatureInputRot)

                # scrapFeatureInputRot2 = scrapFeatures.createInput(scrapCollection, scrapRotationMatrix2)
                # scrapFeatures.add(scrapFeatureInputRot2)

                # if (modelCenterTranslationMatrix.translation.length > 0.00001):
                #     scrapFeatureInputModelCenT = scrapFeatures.createInput(scrapCollection, modelCenterTranslationMatrix)
                #     scrapFeatures.add(scrapFeatureInputModelCenT)

                # if (scrapTranslationMatrix.translation.length > 0.00001):
                #     scrapFeatureInputT = scrapFeatures.createInput(scrapCollection, scrapTranslationMatrix)
                #     scrapFeatures.add(scrapFeatureInputT)
                
                # if (scrapCenterTranslationMatrix.translation.length > 0.00001):
                #     scrapFeatureInputCenT = scrapFeatures.createInput(scrapCollection, scrapCenterTranslationMatrix)
                #     scrapFeatures.add(scrapFeatureInputCenT)

                # print("Number of meshbodies: " + str(len(root.meshBodies)) + " - " + str(len(modelMeshList)) + " - " + str(len(scrapMeshList)))
                # inputentities.add(root.meshBodies.item(1))

                '''


                # coords = mesh.mesh.nodeCoordinatesAsDouble
                # xmin = coords[0]       
                # ymin = coords[1]
                # zmin = coords[2]
                # i = 3
                # while i < len(coords):
                #     if xmin > coords[i]:
                #         xmin = coords[i]
                #     i += 1
                #     if ymin > coords[i]:
                #         ymin = coords[i]
                #     i += 1
                #     if zmin > coords[i]:
                #         zmin = coords[i]
                #     i += 1
                # ui.messageBox('xmin: {}, ymin: {}, zmin: {}'.format(xmin, ymin, zmin))

            except:
                if ui:
                    ui.messageBox('Adding mesh failed:\n{}'.format(traceback.format_exc()))

            if (False):
                # Post-process the first setup with the updated translation file to generate G-code
                setup = cam.setups.item(0)

                script_dir = os.path.dirname(__file__) #<-- absolute dir the script is in
                rel_path_config = "Resources/prusa.cps"
                abs_file_path_config = os.path.join(script_dir, rel_path_config)


                programName = 'PrusaGCODE'
                outputFolder = 'D:/TEMP'
                postConfig = abs_file_path_config
                units = adsk.cam.PostOutputUnitOptions.DocumentUnitsOutput
                postInput = adsk.cam.PostProcessInput.create(programName, postConfig, outputFolder, units)
                postInput.isOpenInEditor = True
                r = cam.postProcess(setup, postInput)
        
        # Output checks
        # _ui.messageBox(str(ports_in_use))
        # subprocess.run(['ping', "8.8.8.8"])
        
        
# Event handler for the commandExecuted event.
class ShowPaletteCommandExecuteHandler(adsk.core.CommandEventHandler):
    def __init__(self):
        super().__init__()
    def notify(self, args):

        HTMLLock.acquire()
        try:
            # Create and display the palette.
            palette = _ui.palettes.itemById('scrapLibraryPalette')
            if not palette:
                palette = _ui.palettes.add('scrapLibraryPalette', 'Scrap Library', 'scrapLibrary.html', True, True, True, 300, 200, True)

                # Dock the palette to the right side of Fusion window.
                palette.dockingState = adsk.core.PaletteDockingStates.PaletteDockStateRight
    
                # Add handler to HTMLEvent of the palette.
                onHTMLEvent = MyHTMLEventHandler()
                palette.incomingFromHTML.add(onHTMLEvent)   
                handlers.append(onHTMLEvent)
    
                # Add handler to CloseEvent of the palette.
                onClosed = MyCloseEventHandler()
                palette.closed.add(onClosed)
                handlers.append(onClosed)   
            else:
                palette.isVisible = True                               
        except:
            _ui.messageBox('Command executed failed: {}'.format(traceback.format_exc()))
        HTMLLock.release()

        initializeHTML()
        updateHTML()

# Event handler for the commandCreated event.
class ShowPaletteCommandCreatedHandler(adsk.core.CommandCreatedEventHandler):
    def __init__(self):
        super().__init__()              
    def notify(self, args):
        try:
            command = args.command
            onExecute = ShowPaletteCommandExecuteHandler()
            command.execute.add(onExecute)
            handlers.append(onExecute)                                     
        except:
            _ui.messageBox('Failed:\n{}'.format(traceback.format_exc()))     


# Event handler for the commandExecuted event.
class SendInfoCommandExecuteHandler(adsk.core.CommandEventHandler):
    def __init__(self):
        super().__init__()
    def notify(self, args):
        try:
            # Send information to the palette. This will trigger an event in the javascript
            # within the html so that it can be handled.
            palette = _ui.palettes.itemById('scrapLibraryPalette')
            if palette:
                global num
                num += 1
                palette.sendInfoToScrapLibraryHTML('send', 'This is a message sent to the palette from Fusion. It has been sent {} times.'.format(num))                        
        except:
            _ui.messageBox('Command executed failed: {}'.format(traceback.format_exc()))


# Event handler for the commandCreated event.
class SendInfoCommandCreatedHandler(adsk.core.CommandCreatedEventHandler):
    def __init__(self):
        super().__init__()              
    def notify(self, args):
        try:
            command = args.command
            onExecute = SendInfoCommandExecuteHandler()
            command.execute.add(onExecute)
            handlers.append(onExecute)                                     
        except:
            _ui.messageBox('Failed:\n{}'.format(traceback.format_exc()))     


# Event handler for the palette close event.
class MyCloseEventHandler(adsk.core.UserInterfaceGeneralEventHandler):
    def __init__(self):
        super().__init__()
    def notify(self, args):
        try:
            _ui.messageBox('Close button is clicked.') 
        except:
            _ui.messageBox('Failed:\n{}'.format(traceback.format_exc()))


# Event handler for the palette HTML event.                
class MyHTMLEventHandler(adsk.core.HTMLEventHandler):
    def __init__(self):
        super().__init__()
    def notify(self, args):
        try:
            htmlArgs = adsk.core.HTMLEventArgs.cast(args)            
            data = json.loads(htmlArgs.data)
            msg = "An event has been fired from the html to Fusion with the following data:\n"
            msg += '    Command: {}\n    arg1: {}\n    arg2: {}'.format(htmlArgs.action, data['arg1'], data['arg2'])
            #_ui.messageBox(msg)
            if (data['arg1'] == "IP"):
                #global target_host
                msg = scrappyClient.target_host
                scrappyClient.target_host = data['arg2']
                msg += " --> New server IP: "
                msg += scrappyClient.target_host
            elif (data['arg1'] == "UpdateHTML"):
                updateHTML()
            elif (data['arg1'] == "UpdateScrap"):
                # TODO: Make this threaded as well
                # Checking if it is the same second to avoid duplicate execution. TODO: Find out why it is duplicated or synchronize this in a better way.
                # It is because the old handlers don't get removed.
                clickTime = data['arg2']
                clickTime = clickTime[:-3]
                clickSecond = int(clickTime[-2:])
                updateTime = datetime.datetime.now()

                updateSecond = int(updateTime.strftime("%S"))
                print(data)
                print("clickTime:" + str(clickTime))
                print("updateTime:" + str(updateTime))
                print(updateSecond)
                print(clickSecond)

                # Now it just stops it from working since the latest handler is not being executed

                # if (abs(updateSecond-clickSecond) > 1 and abs(updateSecond-clickSecond) < 59):
                #     print("Prevented execution due to large time difference")
                # else:
                #     print("Updating all scrap (User click received)")
                #     updateAllScrap(updateTime)
                updateAllScrap(updateTime)
            elif (data['arg1'] == "PreviewScrap"):
                for scrapItem in scrapList:
                    if (data['arg2'] == scrapItem.ID):
                        previewScrap(scrapItem)
                        break
            elif (data['arg1'] == "IntegrateScrap"):
                for scrapItem in scrapList:
                    if (data['arg2'] == scrapItem.ID):
                        integrateScrap(scrapItem)
                        break
            elif (data['arg1'] == "RequestScrap"):
                for scrapItem in scrapList:
                    if (data['arg2'] == scrapItem.ID):
                        requestMesh(scrapItem)
                        break
            # _ui.messageBox(msg)
            
            #scrappyClient.start_client()
        except:
            _ui.messageBox('Failed:\n{}'.format(traceback.format_exc()))           

# When the addin stops we need to clean up the ui
def stop(context):
    # Notify threads it's time to exit
    global exitFlag
    exitFlag = 1


    app = adsk.core.Application.get()
    ui = app.userInterface



    try:
    
        # Delete the palette created by this add-in.
        palette = _ui.palettes.itemById('scrapLibraryPalette')
        if palette:
            palette.deleteMe()
            
            
        for handler in handlers: # I think this is all doing nothing.
            if handler:
                try: 
                    app.EventTypeObject.remove(handler)
                except:
                    pass
                try: 
                    handler.deleteMe()
                except:
                    pass
                try: 
                    del handler
                except:
                    pass
                # MyHTMLEventHandler ShowPaletteCommandExecuteHandler
    
        # Clean up the UI by deleting the button.
        cmdDef = ui.commandDefinitions.itemById('buttonMatryoshkaDefIdPython')
        if cmdDef:
            cmdDef.deleteMe()
            
        cmdDef = ui.commandDefinitions.itemById('showScrapLibraryPalette')
        if cmdDef:
            cmdDef.deleteMe()
            
        cmdDef = ui.commandDefinitions.itemById('sendInfoToScrapLibraryHTML')
        if cmdDef:
            cmdDef.deleteMe()
            
            
        addinsPanel = ui.allToolbarPanels.itemById('SolidScriptsAddinsPanel')
        cntrl = addinsPanel.controls.itemById('buttonMatryoshkaDefIdPython')
        if cntrl:
            cntrl.deleteMe()
        
        ''' # handled by code below
        cntrl = panel.controls.itemById('showScrapLibraryPalette')
        if cntrl:
            cntrl.deleteMe()
            
        cntrl = panel.controls.itemById('sendInfoToScrapLibraryHTML')
        if cntrl:
            cntrl.deleteMe()
        '''
            
        # Get all the toolbar panels
        allToolbarPanels = ui.allToolbarPanels

        # See if our design panel still exists
        scrappyFabPanel = allToolbarPanels.itemById('AddInShortcut_PanelId')
        samplePanel = allToolbarPanels.itemById('Matryoshka_PanelId')

        if scrappyFabPanel is not None:

            # Remove the controls we added to our panel
            for control in scrappyFabPanel.controls:
                if control.isValid:
                    control.deleteMe()

            # Remove our panel
            scrappyFabPanel.deleteMe()

        # See if our design panel still exists
        if samplePanel is not None:

            # Remove the controls we added to our panel
            for control in samplePanel.controls:
                if control.isValid:
                    control.deleteMe()

            # Remove our panel
            samplePanel.deleteMe()

        # Get all of the toolbar tabs
        allToolbarTabs = ui.allToolbarTabs

        # See if our design tab still exists
        scrappyTab = allToolbarTabs.itemById('ScrappyFabrication_DesignTab')
        if scrappyFabPanel is not None:

            # Remove our design tab from the UI
            if scrappyTab.isValid:
                scrappyTab.deleteMe()
                
        try:
            adsk.terminate() # handlers still persist
        except:
            pass

    except:
        if ui:
            ui.messageBox('Remove All::Failed:\n{}'.format(traceback.format_exc()))
