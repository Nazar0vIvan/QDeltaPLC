import QtQml

QtObject {
  // Set identity once when constructing an object; selection uses it, never name.
  required property string objectId
  required property string name
  required property int kind

  enum Kind { Plane, Cylinder, Cone, Edge, ScanPath, MachiningPath }
  enum Classification { Unclassified, Rough, Precise }

  property int classification: SceneObject.Unclassified
  property bool visible: true
  property url sourceUrl: ""
  // A concrete geometry type will be introduced alongside its producer.
  property QtObject geometry: null
}
