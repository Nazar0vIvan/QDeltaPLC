import QtQml

QtObject {
  id: root

  default property list<SceneObject> objects

  property int nextPlaneId: 1
  readonly property Component planeFactory: Component {
    SceneObject {
      id: plane
      required property list<real> coefficients
      kind: SceneObject.Plane
      classification: SceneObject.Rough
      geometry: PlaneGeometry {
        normalX: plane.coefficients[0]
        normalY: plane.coefficients[1]
        normalZ: plane.coefficients[2]
        offset: plane.coefficients[3]
      }
    }
  }

  function addPlane(sourceUrl: url, name: string, coefficients: list<real>): SceneObject {
    if (coefficients.length !== 4 || sourceUrl.toString().length === 0)
      return null
    for (const value of coefficients) {
      if (!isFinite(value))
        return null
    }
    const norm = Math.hypot(coefficients[0], coefficients[1], coefficients[2])
    if (Math.abs(norm - 1) > 1e-6)
      return null

    let objectId
    do {
      objectId = "imported-plane-" + root.nextPlaneId++
    } while (root.findObject(objectId))

    const object = root.planeFactory.createObject(root, {
      objectId: objectId,
      name: name.trim() || qsTr("Plane"),
      sourceUrl: sourceUrl,
      coefficients: coefficients
    })
    if (!object)
      return null
    const objects = root.objects.slice()
    objects.push(object)
    root.objects = objects
    return object
  }

  function findObject(objectId: string): SceneObject {
    for (const object of root.objects) {
      if (object.objectId === objectId)
        return object
    }
    return null
  }

  function renameObject(object: SceneObject, name: string): bool {
    const trimmedName = name.trim()
    if (!object || root.objects.indexOf(object) < 0 || trimmedName.length === 0)
      return false
    object.name = trimmedName
    return true
  }

  function setObjectVisible(object: SceneObject, visible: bool): bool {
    if (!object || root.objects.indexOf(object) < 0)
      return false
    object.visible = visible
    return true
  }
}
