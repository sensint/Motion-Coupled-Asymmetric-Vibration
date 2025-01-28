using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Experimental.XR.Interaction;

public enum MagneticMode
{
    NONE = 0,
    ATTRACT = 1,
    REPEL = 2
}

public class InteractionMonitoredMagnet : MonoBehaviour, ISerialObject
{
    private SceneSettings _sceneSettings;

    public GameObject originalPosition;

    public float multiplier = 1.0f;

    public float activationDistance = 0.30f;

    public float waitTime = 0.1f;

    public MagneticMode magneticMode = MagneticMode.NONE;
    public float relativeDistance = 0.0f;



    public Transform positivePole;  // The positive pole of the magnet (south)
    public Transform negativePole;  // The negative pole of the magnet (north)

    private Coroutine detectionCoroutine;

    void Awake()
    {
        _sceneSettings = GameObject.Find("Scene Settings").GetComponent<SceneSettings>();

        // positivePole = transform.Find("south");
        // negativePole = transform.Find("north");

        detectionCoroutine = StartCoroutine(DetectNearbyMagnets());
    }

    void OnDestroy()
    {
        if (detectionCoroutine != null)
        {
            StopCoroutine(detectionCoroutine);
        }
    }

    void Update()
    {

    }

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
        if (magneticMode == MagneticMode.NONE)
        {
            return 0.0f;
        }
        if (magneticMode == MagneticMode.ATTRACT)
        {
            return relativeDistance;
        }
        else
        {
            return relativeDistance * -1.0f;
        }
    }

    public float getMultiplier()
    {
        return multiplier;
    }


    IEnumerator DetectNearbyMagnets()
    {
        while (true)
        {
            bool foundOne = false;
            InteractionMonitoredMagnet[] allMagnets = FindObjectsOfType<InteractionMonitoredMagnet>();
            foreach (var otherMagnet in allMagnets)
            {
                if (otherMagnet != this)
                {
                    float distance = Vector3.Distance(transform.position, otherMagnet.transform.position);
                    if (distance <= activationDistance)
                    {
                        // relativeDistance = 1.0f - (distance / activationDistance);
                        foundOne = true;
                        DetermineInteraction(otherMagnet);
                    }
                }
            }

            if (!foundOne)
            {
                relativeDistance = 0.0f;
                magneticMode = MagneticMode.NONE;
            }

            // if (_sceneSettings.debug)
            // {
            //     Debug.Log($"{gameObject.name} is {this.magneticMode} at {relativeDistance}.");
            // }


            yield return new WaitForSeconds(waitTime);
        }
    }

    void DetermineInteraction(InteractionMonitoredMagnet otherMagnet)
    {
        // Determine which pole of the current magnet is closer to the other magnet
        float distanceToPositivePole = Vector3.Distance(positivePole.position, otherMagnet.transform.position);
        float distanceToNegativePole = Vector3.Distance(negativePole.position, otherMagnet.transform.position);
        float distanceToOtherPositivePole = Vector3.Distance(otherMagnet.positivePole.position, transform.position);
        float distanceToOtherNegativePole = Vector3.Distance(otherMagnet.negativePole.position, transform.position);
        bool otherMagnetPositiveIsCloser = distanceToOtherPositivePole < distanceToOtherNegativePole;

        Collider southCollider = transform.Find("south").GetComponent<Collider>();
        Collider northCollider = transform.Find("north").GetComponent<Collider>();
        Collider southOtherCollider = otherMagnet.transform.Find("south").GetComponent<Collider>();
        Collider northOtherCollider = otherMagnet.transform.Find("north").GetComponent<Collider>();

        // relativeDistance = 1.0f - (distance / activationDistance);

        Vector3 closestPointOnThis;
        Vector3 closestPointOnOther;

        if (distanceToPositivePole < distanceToNegativePole)
        {
            closestPointOnThis = southCollider.ClosestPoint(otherMagnet.transform.position);
            if (otherMagnetPositiveIsCloser)
            {
                closestPointOnOther = southOtherCollider.ClosestPoint(transform.position);
                // Repel();
                this.magneticMode = MagneticMode.REPEL;
            }
            else
            {
                closestPointOnOther = northOtherCollider.ClosestPoint(transform.position);
                this.magneticMode = MagneticMode.ATTRACT;
            }
        }
        else
        {
            closestPointOnThis = northCollider.ClosestPoint(otherMagnet.transform.position);
            if (otherMagnetPositiveIsCloser)
            {
                closestPointOnOther = southOtherCollider.ClosestPoint(transform.position);
                // Attract();
                this.magneticMode = MagneticMode.ATTRACT;
            }
            else
            {
                closestPointOnOther = northOtherCollider.ClosestPoint(transform.position);
                this.magneticMode = MagneticMode.REPEL;
                // Repel();
            }
        }

        float edgeDistance = Vector3.Distance(closestPointOnThis, closestPointOnOther);
        if (edgeDistance <= activationDistance)
        {
            relativeDistance = 1.0f - (edgeDistance / activationDistance);
        }
        
    }

    // void CheckAttractionOrRepulsion(InteractionMonitoredMagnet otherMagnet, Transform currentPole, bool isPositivePole)
    // {
    //     // Get the closest pole of the other magnet
    //     float distanceToOtherPositivePole = Vector3.Distance(otherMagnet.positivePole.position, transform.position);
    //     float distanceToOtherNegativePole = Vector3.Distance(otherMagnet.negativePole.position, transform.position);

    //     bool otherMagnetPositiveIsCloser = distanceToOtherPositivePole < distanceToOtherNegativePole;

    //     if (isPositivePole)
    //     {
    //         if (otherMagnetPositiveIsCloser)
    //         {
    //             Repel();
    //         }
    //         else
    //         {
    //             Attract();
    //         }
    //     }
    //     else
    //     {
    //         if (otherMagnetPositiveIsCloser)
    //         {
    //             Attract();
    //         }
    //         else
    //         {
    //             Repel();
    //         }
    //     }
    // }

    // void Attract()
    // {
    //     // if (hapticCoroutine == null)
    //     // {
    //     //     hapticCoroutine = StartCoroutine(Vibrate());
    //     // }
    //     // Attraction logic (e.g., move closer)
    //     Debug.Log($"{gameObject.name} is attracting.");
    // }

    // void Repel()
    // {
    //     // if (hapticCoroutine == null)
    //     // {
    //     //     hapticCoroutine = StartCoroutine(Vibrate());
    //     // }
    //     // Repulsion logic (e.g., move away)
    //     Debug.Log($"{gameObject.name} is repelling.");
    // }
}
