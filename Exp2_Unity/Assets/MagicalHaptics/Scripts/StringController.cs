using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.XR.Interaction.Toolkit;
using UnityEngine.XR.Interaction.Toolkit.Interactables;

public class StringController : MonoBehaviour, ISerialObject
{
    [SerializeField]
    private BowString bowStringRenderer;

    private XRGrabInteractable interactable;

    [SerializeField]
    private Transform midPointGrabObject, midPointVisualObject, midPointParent;

    [SerializeField]
    private float bowStringStretchLimit = 0.3f;

    private Transform interactor;

    private float strength, previousStrength;

    [SerializeField]
    private float stringSoundThreshold = 0.001f;

    [SerializeField]
    private AudioSource audioSource;

    public UnityEvent OnBowPulled;
    public UnityEvent<float> OnBowReleased;

    private bool isBowPulled = false;

    private void Awake()
    {
        interactable = midPointGrabObject.GetComponent<XRGrabInteractable>();
    }

    private void Start()
    {
        interactable.selectEntered.AddListener(PrepareBowString);
        interactable.selectExited.AddListener(ResetBowString);
    }

    private void ResetBowString(SelectExitEventArgs arg0)
    {
        OnBowReleased?.Invoke(strength);
        strength = 0;
        previousStrength = 0;
        if (audioSource != null)
        {
            audioSource.pitch = 1;
            audioSource.Stop();
        }

        isBowPulled = false;

        interactor = null;
        midPointGrabObject.localPosition = Vector3.zero;
        midPointVisualObject.localPosition = Vector3.zero;
        bowStringRenderer.CreateString(null);
    }

    private void PrepareBowString(SelectEnterEventArgs arg0)
    {
        interactor = arg0.interactorObject.transform;
        isBowPulled = true;
        OnBowPulled?.Invoke();
    }

    private void Update()
    {
        if (interactor != null)
        {
            //convert bow string mid point position to the local space of the MidPoint
            Vector3 midPointLocalSpace =
                midPointParent.InverseTransformPoint(midPointGrabObject.position); // localPosition

            //get the offset
            float midPointLocalZAbs = Mathf.Abs(midPointLocalSpace.z);


            previousStrength = strength;

            HandleStringPushedBackToStart(midPointLocalSpace);

            HandleStringPulledBackTolimit(midPointLocalZAbs, midPointLocalSpace);

            HandlePullingString(midPointLocalZAbs, midPointLocalSpace);

            bowStringRenderer.CreateString(midPointVisualObject.position);
        }
    }

    private void HandlePullingString(float midPointLocalZAbs, Vector3 midPointLocalSpace)
    {
        //what happens when we are between point 0 and the string pull limit
        if (midPointLocalSpace.z < 0 && midPointLocalZAbs < bowStringStretchLimit)
        {
            if (audioSource != null && audioSource.isPlaying == false && strength <= 0.01f)
            {
                audioSource.Play();
            }

            strength = Remap(midPointLocalZAbs, 0, bowStringStretchLimit, 0, 1);
            midPointVisualObject.localPosition = new Vector3(0, 0, midPointLocalSpace.z);

            PlayStringPullinSound();
        }
    }

    private void PlayStringPullinSound()
    {
        if (audioSource == null)
            return;
        //Check if we have moved the string enought to play the sound unpause it
        if (Mathf.Abs(strength - previousStrength) > stringSoundThreshold)
        {
            if (strength < previousStrength)
            {
                //Play string sound in reverse if we are pusing the string towards the bow
                audioSource.pitch = -1;
            }
            else
            {
                //Play the sound normally
                audioSource.pitch = 1;
            }
            audioSource.UnPause();
        }
        else
        {
            //if we stop moving Pause the sounds
            audioSource.Pause();
        }

    }

    private float Remap(float value, int fromMin, float fromMax, int toMin, int toMax)
    {
        return (value - fromMin) / (fromMax - fromMin) * (toMax - toMin) + toMin;
    }

    private void HandleStringPulledBackTolimit(float midPointLocalZAbs, Vector3 midPointLocalSpace)
    {
        //We specify max pulling limit for the string. We don't allow the string to go any farther than "bowStringStretchLimit"
        if (midPointLocalSpace.z < 0 && midPointLocalZAbs >= bowStringStretchLimit)
        {
            if (audioSource != null)
            {
                audioSource.Pause();
            }
            strength = 1;
            //Vector3 direction = midPointParent.TransformDirection(new Vector3(0, 0, midPointLocalSpace.z));
            midPointVisualObject.localPosition = new Vector3(0, 0, -bowStringStretchLimit);
        }
    }

    private void HandleStringPushedBackToStart(Vector3 midPointLocalSpace)
    {
        if (midPointLocalSpace.z >= 0)
        {
            if (audioSource != null)
            {
                audioSource.pitch = 1;
                audioSource.Stop();
            }
            strength = 0;
            midPointVisualObject.localPosition = Vector3.zero;
        }
    }

    public bool hasMessage()
    {
        return true;
    }

    public string composeMessage()
    {
        string state = isBowPulled ? "1" : "0";
        string message = $"B;{state};{strength}";
        return message;
    }

    public float getInteractionValue()
    {
        if(isBowPulled)
        {
            return Math.Clamp(strength, 0.3f, 1.0f);
        }
        return strength;
    }

    public float getMultiplier()
    {
        return 1.0f;
    }
}