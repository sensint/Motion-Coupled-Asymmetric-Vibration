using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.XR.Interaction.Toolkit;
using UnityEngine.XR.Interaction.Toolkit.Interactables;
using UnityEngine.XR.Interaction.Toolkit.Interactors;

[RequireComponent(typeof(HapticManagerMagnets))]
public class InteractionMonitorMagnets : MonoBehaviour
{
    private SceneSettings _sceneSettings;


    public List<XRGrabInteractable> monitoredObjects;

    private HapticManagerMagnets _hapticManager;

    private ISerialObject activeObjectLeft;
    private bool isGrabbedLeft;
    private bool isHoveringLeft;

    private ISerialObject activeObjectRight;
    private bool isGrabbedRight;
    private bool isHoveringRight;

    private InteractorHandedness activeHandedness;


    void Awake()
    {
        _sceneSettings = GameObject.Find("Scene Settings").GetComponent<SceneSettings>();

        _hapticManager = this.gameObject.GetComponent<HapticManagerMagnets>();
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

        if (activeHandedness == InteractorHandedness.Left)
        {
            Debug.Log($"Object Left: {args.interactableObject} => {args.interactableObject.transform} => {args.interactableObject.transform.gameObject} => {args.interactableObject.transform.gameObject.GetComponent<ISerialObject>()}");

            activeObjectLeft = args.interactableObject.transform.gameObject.GetComponent<ISerialObject>();
            isGrabbedLeft = true;

            _hapticManager.StartGrab(activeObjectLeft, activeHandedness);

        }
        else if(activeHandedness == InteractorHandedness.Right)
        {
            Debug.Log($"Object Right: {args.interactableObject} => {args.interactableObject.transform} => {args.interactableObject.transform.gameObject} => {args.interactableObject.transform.gameObject.GetComponent<ISerialObject>()}");

            activeObjectRight = args.interactableObject.transform.gameObject.GetComponent<ISerialObject>();
            isGrabbedRight = true;

            _hapticManager.StartGrab(activeObjectRight, activeHandedness);
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
        if (activeHandedness == InteractorHandedness.Left)
        {
            Debug.Log($"Object Left: {args.interactableObject} => {args.interactableObject.transform} => {args.interactableObject.transform.gameObject} => {args.interactableObject.transform.gameObject.GetComponent<ISerialObject>()}");

            activeObjectLeft = args.interactableObject.transform.gameObject.GetComponent<ISerialObject>();
            isGrabbedLeft = false;

            _hapticManager.StopGrab(activeObjectLeft, activeHandedness);

        }
        else if(activeHandedness == InteractorHandedness.Right)
        {
            Debug.Log($"Object Right: {args.interactableObject} => {args.interactableObject.transform} => {args.interactableObject.transform.gameObject} => {args.interactableObject.transform.gameObject.GetComponent<ISerialObject>()}");

            activeObjectRight = args.interactableObject.transform.gameObject.GetComponent<ISerialObject>();
            isGrabbedRight = false;

            _hapticManager.StopGrab(activeObjectRight, activeHandedness);
        }
    }

    void OnHoverEntered(HoverEnterEventArgs args)
    {
        if (_sceneSettings.debug)
        {
            Debug.Log($"Hovering over: {args.interactableObject.transform.gameObject.name}");
        }

        activeHandedness = args.interactorObject.handedness;
        if (activeHandedness == InteractorHandedness.Left)
        {
            Debug.Log($"Object Left: {args.interactableObject} => {args.interactableObject.transform} => {args.interactableObject.transform.gameObject} => {args.interactableObject.transform.gameObject.GetComponent<ISerialObject>()}");

            activeObjectLeft = args.interactableObject.transform.gameObject.GetComponent<ISerialObject>();
            isHoveringLeft = true;

            _hapticManager.StartHover(activeObjectLeft, activeHandedness);

        }
        else if(activeHandedness == InteractorHandedness.Right)
        {
            Debug.Log($"Object Right: {args.interactableObject} => {args.interactableObject.transform} => {args.interactableObject.transform.gameObject} => {args.interactableObject.transform.gameObject.GetComponent<ISerialObject>()}");

            activeObjectRight = args.interactableObject.transform.gameObject.GetComponent<ISerialObject>();
            isHoveringRight = true;

            _hapticManager.StartHover(activeObjectRight, activeHandedness);
        }
    }

    void OnHoverExited(HoverExitEventArgs args)
    {
        if (_sceneSettings.debug)
        {
            Debug.Log($"Stopped hovering over: {args.interactableObject.transform.gameObject.name}");
        }
        activeHandedness = args.interactorObject.handedness;
        if (activeHandedness == InteractorHandedness.Left)
        {
            Debug.Log($"Object Left: {args.interactableObject} => {args.interactableObject.transform} => {args.interactableObject.transform.gameObject} => {args.interactableObject.transform.gameObject.GetComponent<ISerialObject>()}");

            activeObjectLeft = args.interactableObject.transform.gameObject.GetComponent<ISerialObject>();
            isHoveringLeft = false;

            _hapticManager.StopHover(activeObjectLeft, activeHandedness);

        }
        else if(activeHandedness == InteractorHandedness.Right)
        {
            Debug.Log($"Object Right: {args.interactableObject} => {args.interactableObject.transform} => {args.interactableObject.transform.gameObject} => {args.interactableObject.transform.gameObject.GetComponent<ISerialObject>()}");

            activeObjectRight = args.interactableObject.transform.gameObject.GetComponent<ISerialObject>();
            isHoveringRight = false;

            _hapticManager.StopHover(activeObjectRight, activeHandedness);
        }
    }

}
