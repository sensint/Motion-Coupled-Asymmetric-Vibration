using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class InteractionMonitoredBox : MonoBehaviour, ISerialObject
{
    public GameObject originalPosition;

    public float multiplier = 1.0f;

    public float scaling = 0.3f;

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
        Vector3 position = this.transform.position;
        Vector3 origPosition = originalPosition.transform.position;
        Vector3 distance = position - origPosition;
        //if(SceneSettings.Instance.debug)
        //{
        //    Debug.Log($"getInteractionValue: {distance.magnitude}, {distance}, {this.transform.position}, {originalPosition.transform.position}");
        //}
        

        float result = Mathf.Clamp(distance.magnitude, 0, scaling);
        float normalizedValue = result / scaling;
        return normalizedValue;
    }

    public float getMultiplier()
    {
        return multiplier;
    }
}
