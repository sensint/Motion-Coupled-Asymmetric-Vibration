using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.XR.Interaction.Toolkit;
using UnityEngine.XR.Interaction.Toolkit.Interactables;
using UnityEngine.XR.Interaction.Toolkit.Interactors;

[RequireComponent(typeof(HapticManager))]
public class InteractionMonitor : MonoBehaviour
{
    private SceneSettings _sceneSettings;


    public List<XRGrabInteractable> monitoredObjects;

    private HapticManager _hapticManager;

    private ISerialObject activeObject;
    private bool isGrabbed;
    private bool isHovering;
    private InteractorHandedness activeHandedness;


    void Awake()
    {
        _sceneSettings = GameObject.Find("Scene Settings").GetComponent<SceneSettings>();

        _hapticManager = this.gameObject.GetComponent<HapticManager>();
        if (_hapticManager == null)
        {
            Debug.LogError($"_hapticManager == null");
            return;
        }

        foreach (XRGrabInteractable grabInteractable in monitoredObjects)
        {
            ISerialObject serialObject = grabInteractable.gameObject.GetComponent<ISerialObject>();
            if (serialObject == null)
            {
                Debug.LogError($"serialObject == null for {grabInteractable.gameObject.name}");
                continue;
            }
            if (_sceneSettings.debug)
            {
                Debug.Log($"Adding listeners to {grabInteractable.gameObject.name}");
            }
            grabInteractable.selectEntered.AddListener(OnSelectEntered);
            grabInteractable.selectExited.AddListener(OnSelectExited);
            grabInteractable.hoverEntered.AddListener(OnHoverEntered);
            grabInteractable.hoverExited.AddListener(OnHoverExited);
        }
    }

    void Update()
    {
        //_hapticManager.sendObjectMessage(activeObject, isGrabbed);
    }

    void OnDestroy()
    {
        foreach (XRGrabInteractable grabInteractable in monitoredObjects)
        {
            if (_sceneSettings.debug)
            {
                Debug.Log($"Removing listeners");
                // Debug.Log($"Removing listeners from {grabInteractable.gameObject.name}");
            }
            grabInteractable.selectEntered.RemoveListener(OnSelectEntered);
            grabInteractable.selectExited.RemoveListener(OnSelectExited);
            grabInteractable.hoverEntered.RemoveListener(OnHoverEntered);
            grabInteractable.hoverExited.RemoveListener(OnHoverExited);
        }
    }


    void OnSelectEntered(SelectEnterEventArgs args)
    {
        if (_sceneSettings.debug)
        {
            Debug.Log($"Grabbed: {args.interactableObject.transform.gameObject.name}");
        }
        // <Scene, Algorithm, State, percent_stretch>;
        activeHandedness = args.interactorObject.handedness;
        //Debug.Log($"Handedness: {_checkHandedness} =? {activeHandedness}");
        if (_sceneSettings.activeMonitoredHand == activeHandedness)
        {
            Debug.Log($"Object: {args.interactableObject} => {args.interactableObject.transform} => {args.interactableObject.transform.gameObject} => {args.interactableObject.transform.gameObject.GetComponent<ISerialObject>()}");

            activeObject = args.interactableObject.transform.gameObject.GetComponent<ISerialObject>();
            isGrabbed = true;

            _hapticManager.StartGrab(activeObject);
        }
    }

    void OnSelectExited(SelectExitEventArgs args)
    {
        if (_sceneSettings.debug)
        {
            Debug.Log($"Released: {args.interactableObject.transform.gameObject.name}");
        }
        // <Scene, Algorithm, State, percent_stretch>;
        activeHandedness = args.interactorObject.handedness;
        if (_sceneSettings.activeMonitoredHand == activeHandedness)
        {
            activeObject = args.interactableObject.transform.gameObject.GetComponent<ISerialObject>();
            isGrabbed = false;

            _hapticManager.StopGrab(activeObject);
        }
    }

    void OnHoverEntered(HoverEnterEventArgs args)
    {
        if (_sceneSettings.debug)
        {
            Debug.Log($"Hovering over: {args.interactableObject.transform.gameObject.name}");
        }
        activeHandedness = args.interactorObject.handedness;
        if (_sceneSettings.activeMonitoredHand == activeHandedness)
        {
            activeObject = args.interactableObject.transform.gameObject.GetComponent<ISerialObject>();
            isHovering = true;

            _hapticManager.StartHover(activeObject);
        }
    }

    void OnHoverExited(HoverExitEventArgs args)
    {
        if (_sceneSettings.debug)
        {
            Debug.Log($"Stopped hovering over: {args.interactableObject.transform.gameObject.name}");
        }
        activeHandedness = args.interactorObject.handedness;
        if (_sceneSettings.activeMonitoredHand == activeHandedness)
        {
            activeObject = args.interactableObject.transform.gameObject.GetComponent<ISerialObject>();
            isHovering = false;

            _hapticManager.StopHover(activeObject);
        }
    }

}
