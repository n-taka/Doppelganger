# -*- coding: utf-8 -*-

import json
import os
import math
# FreeCAD modules
import FreeCAD
import Import
import Mesh
import Part
# import MeshPart

pathToItself = os.path.dirname(os.path.realpath(__file__))
pathToConfig = pathToItself + "/FreeCADConfig.json"

j = {}
with open(pathToConfig, encoding='utf-8') as f:
    j = json.load(f, encoding="utf-8")

meshName = j["meshName"]
filePath = j["filePath"]
merge = j["merge"]
maxTolerance = j["maxTolerance"]
dirPath = filePath+".dump"

os.makedirs(dirPath, exist_ok=True)

# Import.open(filePath) : name of parts are kept, but conversion to mesh fails
# Part.open(filePath)   : name of parts are NOT kept, but conversion to mesh works
# Part.insert(filePath) : name of parts are NOT kept, but conversion to mesh works

meshNameList = []

# names of the meshes
if not merge:
    Import.open(filePath)
    objects = FreeCAD.ActiveDocument.Objects
    meshNameList = list(map((lambda x: x.Label), filter(
        (lambda x: type(x) == Part.Feature), objects)))

# re-open for export
Part.open(filePath)
objects = FreeCAD.ActiveDocument.Objects

# if len(nameList) != len(objects), we use sequential number
if (len(meshNameList) != len(objects)):
    meshNameList = list(map((lambda x: meshName+str(x)), range(len(objects))))

if merge:
    Mesh.export(objects, dirPath+"/"+meshName+".ply", tolerance=maxTolerance)
else:
    for i in range(len(objects)):
        Mesh.export([objects[i]], dirPath+"/" +
                    meshNameList[i]+".ply", tolerance=maxTolerance)
