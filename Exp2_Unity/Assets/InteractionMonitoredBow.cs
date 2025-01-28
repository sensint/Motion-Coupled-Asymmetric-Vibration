using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class InteractionMonitoredBow : MonoBehaviour, ISerialObject
{
    public GameObject bowObject;

    public float multiplier = 1.0f;

    public string composeMessage()
    {
        return "";
    }

    public bool hasMessage()
    {
        return true;
    }

    public float getInteractionValue()
    {
        if (bowObject == null || bowObject.GetComponent<ISerialObject>() == null)
        {
            Debug.LogError($"bowObject == null || bowObject.GetComponent<ISerialObject>() == null");
            return 0.0f;
        }
        return bowObject.GetComponent<ISerialObject>().getInteractionValue();
        // Vector3 position = this.transform.position;
        // Vector3 origPosition = this.transform.position;
        // Vector3 distance = position - origPosition;
        // //if(SceneSettings.Instance.debug)
        // //{
        // //    Debug.Log($"getInteractionValue: {distance.magnitude}, {distance}, {this.transform.position}, {originalPosition.transform.position}");
        // //}
        // return distance.magnitude;
    }

    public float getMultiplier()
    {
        return multiplier;
    }
}
