using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ResetManager : MonoBehaviour
{
    [SerializeField]
    public List<GameObject> monitoredObjects = new List<GameObject>();

    private Dictionary<GameObject, Vector3> originalPositions = new Dictionary<GameObject, Vector3>();
    private Dictionary<GameObject, Quaternion> originalRotations = new Dictionary<GameObject, Quaternion>();

    void Awake()
    {
        foreach(GameObject obj in monitoredObjects)
        {
            originalPositions.Add(obj, obj.transform.position);
            originalRotations.Add(obj, obj.transform.rotation);
        }
    }

    void Update()
    {
        if(Input.GetKeyDown(KeyCode.R))
        {
            this.resetObjects();
        }
    }

    void resetObjects()
    {
        foreach(GameObject obj in monitoredObjects)
        {
            obj.transform.rotation = originalRotations[obj];
            obj.transform.position = originalPositions[obj];
        }
    }
}
