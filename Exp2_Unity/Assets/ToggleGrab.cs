using UnityEngine;
using UnityEngine.XR.Interaction.Toolkit;
using UnityEngine.XR.Interaction.Toolkit.Interactables;
using UnityEngine.XR.Interaction.Toolkit.Interactors;

public class ToggleGrab : MonoBehaviour
{
    // private XRGrabInteractable grabInteractable;
    // private XRBaseInteractor interactor;
    // private bool isGrabbed = false;

    // void Awake()
    // {
    //     grabInteractable = GetComponent<XRGrabInteractable>();
    // }

    // private void OnEnable()
    // {
    //     grabInteractable.selectEntered.AddListener(OnGrab);
    //     grabInteractable.selectExited.AddListener(OnRelease);
    // }

    // private void OnDisable()
    // {
    //     grabInteractable.selectEntered.RemoveListener(OnGrab);
    //     grabInteractable.selectExited.RemoveListener(OnRelease);
    // }

    // private void OnGrab(SelectEnterEventArgs args)
    // {
    //     if (isGrabbed)
    //     {
    //         // If already grabbed, release the object
    //         interactor.interactionManager.SelectExit(interactor, grabInteractable);
    //         isGrabbed = false;
    //     }
    //     else
    //     {
    //         // Otherwise, grab the object
    //         interactor = args.interactorObject;
    //         isGrabbed = true;
    //     }
    // }

    // private void OnRelease(SelectExitEventArgs args)
    // {
    //     // Handle additional logic if needed when the object is released
    //     if (!isGrabbed)
    //     {
    //         interactor = null;
    //     }
    // }
}