using System;
using System.Collections;
using System.Collections.Generic;
using UnityEditor.SearchService;
using UnityEngine;
using UnityEngine.InputSystem.XR;
using UnityEngine.XR.Interaction.Toolkit.Inputs.Haptics;
using UnityEngine.XR.Interaction.Toolkit.Interactors;
using UnityEngine.SceneManagement;

[RequireComponent(typeof(SerialCommunicator))]
public class HapticManagerMagnets : MonoBehaviour, IHapticManager
{
    private SceneSettings _sceneSettings;

    [SerializeField]
    public HapticImpulsePlayer _playerRightController;

    [SerializeField]
    public HapticImpulsePlayer _playerLeftController;

    private SerialCommunicator _serialCommunicator;

    private Coroutine hapticCoroutine;


    private ISerialObject _activeObjectLeft = null;
    private bool _isGrabbedLeft = false;
    private bool _isHoverLeft = false;

    private ISerialObject _activeObjectRight = null;
    private bool _isGrabbedRight = false;
    private bool _isHoverRight = false;


    private float loopDuration = 0.1f;

    private bool sceneUnloaded = false;

    void Awake()
    {
        sceneUnloaded = false;

        _sceneSettings = GameObject.Find("Scene Settings").GetComponent<SceneSettings>();

        _serialCommunicator = this.gameObject.GetComponent<SerialCommunicator>();
        if (_serialCommunicator == null)
        {
            Debug.LogError($"_serialCommunicator == null");
            return;
        }
    }


    // Start is called before the first frame update
    void Start()
    {
        loopDuration = _sceneSettings.loopDuration;
        hapticCoroutine = StartCoroutine(StartHapticFeedbackLoop());
    }

    // Update is called once per frame
    void Update()
    {

    }

    void OnDestroy()
    {
        if (hapticCoroutine != null)
        {
            StopCoroutine(hapticCoroutine);
            hapticCoroutine = null;
        }
    }

    public void StartGrab(ISerialObject obj, InteractorHandedness handedness)
    {
        if (_sceneSettings.debug)
        {
            Debug.Log($"Starting Controller Vibrations: {obj}");
        }

        if (handedness == InteractorHandedness.Left)
        {
            _activeObjectLeft = obj;
            _isGrabbedLeft = true;
        }
        else if (handedness == InteractorHandedness.Right)
        {
            _activeObjectRight = obj;
            _isGrabbedRight = true;
        }
    }

    public void StopGrab(ISerialObject obj, InteractorHandedness handedness)
    {
        if (_sceneSettings.debug)
        {
            Debug.Log($"Stopping Controller Vibrations");
        }

        if (handedness == InteractorHandedness.Left)
        {
            _activeObjectLeft = null;
            _isGrabbedLeft = false;

            if (_sceneSettings.activeAlgorithm == ActuationAlgorithm.CONTROLLER)
            {
                StopHapticFeedback(InteractorHandedness.Left);
            }
        }
        else if (handedness == InteractorHandedness.Right)
        {
            _activeObjectRight = null;
            _isGrabbedRight = false;

            if (_sceneSettings.activeAlgorithm == ActuationAlgorithm.CONTROLLER)
            {
                StopHapticFeedback(InteractorHandedness.Right);
            }
        }
    }


    public void StartHover(ISerialObject obj, InteractorHandedness handedness)
    {
        if (handedness == InteractorHandedness.Left)
        {
            //_activeObjectLeft = obj;
            _isHoverLeft = true;
        }
        else if (handedness == InteractorHandedness.Right)
        {
            //_activeObjectRight = obj;
            _isHoverRight = true;
        }
    }

    public void StopHover(ISerialObject obj, InteractorHandedness handedness)
    {
        if (handedness == InteractorHandedness.Left)
        {
            //_activeObjectLeft = obj;
            _isHoverLeft = false;
        }
        else if (handedness == InteractorHandedness.Right)
        {
            //_activeObjectRight = obj;
            _isHoverRight = false;
        }
    }

    IEnumerator StartHapticFeedbackLoop()
    {
        if (_sceneSettings.debug)
        {
            Debug.Log($"StartHapticFeedbackLoop");
        }
        while (true)
        {
            if (sceneUnloaded)
            {
                break;
            }
            // if (_debug)
            // {
            //     Debug.Log($"while (true)");
            // }
            if (_sceneSettings.activeAlgorithm == ActuationAlgorithm.CONTROLLER)
            {
                // Debug.Log($"_activeAlgorithm == ActuationAlgorithm.CONTROLLER");
                float duration = _sceneSettings.hapticDuration;

                // Debug.Log($"_activeObject {_activeObject}");
                // Debug.Log($"_isGrabbed {_isGrabbed}");

                if (_isGrabbedLeft)
                {
                    if (_activeObjectLeft != null)
                    {
                        //Debug.Log($"_isGrabbed");
                        // calculate amplitude
                        float objectValue = _activeObjectLeft.getInteractionValue();
                        objectValue = Math.Abs(objectValue);
                        // float objectValue = 1.0f;
                        float intensity = objectValue * _sceneSettings.baseAmplitude;
                        if (_sceneSettings.activeControllerHaptics == HapticRenderingFunction.CUBIC)
                        {
                            intensity = CubicMapping(objectValue) * _sceneSettings.baseAmplitude;
                        }

                        if (_sceneSettings.debug)
                        {
                            Debug.Log($"SendHapticImpulse Left: {intensity}, {duration}");
                        }

                        _playerLeftController.SendHapticImpulse(intensity, duration + 0.1f);
                    }
                }

                if (_isGrabbedRight)
                {
                    if (_activeObjectRight != null)
                    {
                        //Debug.Log($"_isGrabbed");
                        // calculate amplitude
                        float objectValue = _activeObjectRight.getInteractionValue();
                        objectValue = Math.Abs(objectValue);
                        // float objectValue = 1.0f;
                        float intensity = objectValue * _sceneSettings.baseAmplitude;
                        if (_sceneSettings.activeControllerHaptics == HapticRenderingFunction.CUBIC)
                        {
                            intensity = CubicMapping(objectValue) * _sceneSettings.baseAmplitude;
                        }

                        if (_sceneSettings.debug)
                        {
                            Debug.Log($"SendHapticImpulse Right: {intensity}, {duration}");
                        }

                        _playerRightController.SendHapticImpulse(intensity, duration + 0.1f);
                    }
                }

                yield return new WaitForSeconds(duration);
            }
            else
            {
                if (_sceneSettings.debug)
                {
                    Debug.Log($"Checking Magnets");
                }
                InteractionMonitoredMagnet leftMagnet = null;
                if (_activeObjectLeft != null && _activeObjectLeft is InteractionMonitoredMagnet)
                {
                    leftMagnet = (InteractionMonitoredMagnet)_activeObjectLeft;
                }
                InteractionMonitoredMagnet rightMagnet = null;
                if (_activeObjectRight != null && _activeObjectRight is InteractionMonitoredMagnet)
                {
                    rightMagnet = (InteractionMonitoredMagnet)_activeObjectRight;
                }

                if (_sceneSettings.debug)
                {
                    Debug.Log($"Left => {leftMagnet}, right => {rightMagnet}");
                }

                int state = 0;
                float value = 0.0f;

                if (leftMagnet != null && rightMagnet != null)
                {
                    value = (leftMagnet.relativeDistance + rightMagnet.relativeDistance) / 2.0f;

                    MagneticMode mode = MagneticMode.NONE;
                    if (leftMagnet.magneticMode != MagneticMode.NONE)
                    {
                        mode = leftMagnet.magneticMode;
                    }
                    else if (rightMagnet.magneticMode != MagneticMode.NONE)
                    {
                        mode = rightMagnet.magneticMode;
                    }

                    state = (int)mode;
                }
                //else if (leftMagnet != null)
                //{
                //    value = leftMagnet.relativeDistance;
                //    state = (int)leftMagnet.magneticMode;
                //}
                //else if (rightMagnet != null)
                //{
                //    value = rightMagnet.relativeDistance;
                //    state = (int)rightMagnet.magneticMode;
                //}
                else
                {
                    value = 0.0f;
                    state = (int)MagneticMode.NONE;
                }

                if (_sceneSettings.activeControllerHaptics == HapticRenderingFunction.CUBIC)
                {
                    value = CubicMapping(value);
                }

                string message = $"{_sceneSettings.GetSceneIdentifier()},{(int)_sceneSettings.activeAlgorithm},{state},{value}";
                if (_sceneSettings.debug)
                {
                    Debug.Log($"SendData: {message}");
                }
                if (_serialCommunicator != null)
                {
                    _serialCommunicator.SendData(message);
                }

                yield return new WaitForSeconds(loopDuration);
            }
            // yield return new WaitForSeconds(0.1f);
        }

    }

    public void sendReset()
    {
        Debug.Log("sendReset()");
        sceneUnloaded = true;
        if (_sceneSettings.activeAlgorithm == ActuationAlgorithm.CONTROLLER)
        {
            StopHapticFeedback(InteractorHandedness.Right);
            StopHapticFeedback(InteractorHandedness.Left);
        }
        else
        {
            string message = $"{_sceneSettings.GetSceneIdentifier()},{(int)_sceneSettings.activeAlgorithm},{0},{0}";
            if (_sceneSettings.debug)
            {
                Debug.Log($"SendData: {message}");
            }
            if (_serialCommunicator != null)
            {
                _serialCommunicator.SendData(message);
            }
        }
    }

    void StopHapticFeedback(InteractorHandedness handedness)
    {
        if (handedness == InteractorHandedness.Left)
        {
            if (_playerLeftController != null)
                _playerLeftController.SendHapticImpulse(0, 0);
        }
        else if (handedness == InteractorHandedness.Right)
        {
            if (_playerRightController != null)
                _playerRightController.SendHapticImpulse(0, 0);
        }
    }

    float CubicMapping(float input)
    {
        // Ensure input is within the range [0, 1]
        input = Mathf.Clamp01(input);

        // Apply the cubic mapping
        float output = Mathf.Pow(input, 3);

        return output;
    }

}
