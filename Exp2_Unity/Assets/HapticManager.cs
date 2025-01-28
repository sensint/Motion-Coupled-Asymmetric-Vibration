using System.Collections;
using System.Collections.Generic;
using UnityEditor.SearchService;
using UnityEngine;
using UnityEngine.InputSystem.XR;
using UnityEngine.XR.Interaction.Toolkit.Inputs.Haptics;
using UnityEngine.XR.Interaction.Toolkit.Interactors;

[RequireComponent(typeof(SerialCommunicator))]
public class HapticManager : MonoBehaviour, IHapticManager
{
    private SceneSettings _sceneSettings;

    // [SerializeField]
    private float vibrationTestDuration = 10.0f;

    private float vibrationTestTimePassed = 0.0f;

    private bool isVibrationTestActive = false;

    private float vibrationTestAmplitude = 1.0f;

    [SerializeField]
    public float sweepTestDuration = 1.0f;

    [SerializeField]
    public float sweepTestStepSize = 0.1f;

    private bool isSweepActive = false;

    private float sweepCurrentAmplitude = 0.0f;

    [SerializeField]
    public HapticImpulsePlayer _playerRightController;

    [SerializeField]
    public HapticImpulsePlayer _playerLeftController;

    private SerialCommunicator _serialCommunicator;

    private Coroutine hapticCoroutine;


    private ISerialObject _activeObject = null;
    private bool _isGrabbed = false;
    private bool _isHover = false;


    private float loopDuration = 0.1f;

    private bool sceneUnloaded = false;

    void Awake()
    {
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
        if(Input.GetKeyDown(KeyCode.F))
        {
            // this.sendFixedVibration(0.4f, vibrationTestDuration);
            this.startVibrationTest(0.4f);
        }
        else if(Input.GetKeyDown(KeyCode.G))
        {
            // this.sendFixedVibration(0.7f, vibrationTestDuration);
            this.startVibrationTest(0.7f);
        }
        else if(Input.GetKeyDown(KeyCode.H))
        {
            // this.sendFixedVibration(1.0f, vibrationTestDuration);
            this.startVibrationTest(1.0f);
        }
        else if(Input.GetKeyDown(KeyCode.S))
        {
            this.toggleSweep();
        }
    }

    // public void sendFixedVibration(float amplitude, float duration)
    // {
    //     this.StopHapticFeedback();
    //     if (_sceneSettings.debug)
    //     {
    //         Debug.Log($"sendFixedVibration: {amplitude}, {duration}");
    //     }
    //     _playerRightController.SendHapticImpulse(amplitude, duration);
    //     _playerLeftController.SendHapticImpulse(amplitude, duration);
    // }

    public void startVibrationTest(float amplitude)
    {
        if (_sceneSettings.debug)
        {
            Debug.Log($"startVibrationTest: {amplitude}");
        }
        vibrationTestAmplitude = amplitude;
        vibrationTestTimePassed = 0.0f;
        isVibrationTestActive = true;
    }

    public void toggleSweep()
    {
        if (isSweepActive)
        {
            isSweepActive = false;
            this.StopHapticFeedback();
        }
        else
        {
            sweepCurrentAmplitude = 0.0f;
            isSweepActive = true;
        }
    }

    void OnDestroy()
    {
        if (hapticCoroutine != null)
        {
            StopCoroutine(hapticCoroutine);
            hapticCoroutine = null;
        }
    }

    public void StartGrab(ISerialObject obj)
    {
        if (_sceneSettings.debug)
        {
            Debug.Log($"Starting Controller Vibrations: {obj}");
        }
        // if (_activeObject != null && _isGrabbed)
        // {
        //     StopGrab(_activeObject);
        // }

        _activeObject = obj;
        _isGrabbed = true;
        //_isHover = false;

        // start haptics
    }

    public void StopGrab(ISerialObject obj)
    {
        if (_sceneSettings.debug)
        {
            Debug.Log($"Stopping Controller Vibrations");
        }
        // Deactivate haptics
        //StopHapticFeedback();

        // reset
        _activeObject = null;
        _isGrabbed = false;
        //_isHover = false;
    }


    public void StartHover(ISerialObject obj)
    {
        // if (_debug)
        // {
        //     Debug.Log($"StartHover");
        // }
        // if (_activeObject != null)
        // {
        //     StopHover(_activeObject);
        // }

        _activeObject = obj;
        //_isGrabbed = false;
        _isHover = true;
    }

    public void StopHover(ISerialObject obj)
    {
        // if (_debug)
        // {
        //     Debug.Log($"StopHover");
        // }
        // Deactivate haptics

        // reset
        //_activeObject = null;
        //_isGrabbed = false;
        _isHover = false;
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

            if (isVibrationTestActive)
            {
                float duration = _sceneSettings.hapticDuration;
                vibrationTestTimePassed += duration;

                if (vibrationTestTimePassed > vibrationTestDuration)
                {
                    isVibrationTestActive = false;
                }
                else
                {
                    _playerRightController.SendHapticImpulse(vibrationTestAmplitude, duration + 0.1f);
                    _playerLeftController.SendHapticImpulse(vibrationTestAmplitude, duration + 0.1f);
                }

                yield return new WaitForSeconds(duration);
            }
            else if (isSweepActive)
            {
                if (_sceneSettings.debug)
                {
                    Debug.Log($"isSweepActive: {sweepCurrentAmplitude}, {sweepTestDuration}");
                }
                _playerRightController.SendHapticImpulse(sweepCurrentAmplitude, sweepTestDuration + 0.1f);
                _playerLeftController.SendHapticImpulse(sweepCurrentAmplitude, sweepTestDuration + 0.1f);
                sweepCurrentAmplitude = sweepCurrentAmplitude + sweepTestStepSize;
                if (sweepCurrentAmplitude > 1.0f)
                {
                    sweepCurrentAmplitude = 0.0f;
                }
                yield return new WaitForSeconds(sweepTestDuration);
            }
            else
            {
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
                if (_activeObject != null)
                {
                    HapticImpulsePlayer toVibrate = _playerRightController;
                    // Actuate controller
                    if (_sceneSettings.activeMonitoredHand == InteractorHandedness.Left)
                    {
                        toVibrate = _playerLeftController;
                    }

                    if (_isGrabbed)
                    {
                        float intensity = 0.0f;
                        if (_sceneSettings.activeSceneName == SceneName.BOXES_WEIGHT)
                        {
                            intensity = _activeObject.getMultiplier();
                        }
                        else
                        {
                            //Debug.Log($"_isGrabbed");
                            // calculate amplitude
                            float objectValue = _activeObject.getInteractionValue();
                            // float objectValue = 1.0f;
                            intensity = objectValue * _sceneSettings.baseAmplitude;
                            if (_sceneSettings.activeControllerHaptics == HapticRenderingFunction.CUBIC)
                            {
                                intensity = CubicMapping(objectValue) * _sceneSettings.baseAmplitude;
                            }
                        }


                        if (_sceneSettings.debug)
                        {
                            Debug.Log($"SendHapticImpulse: {intensity}, {duration}");
                        }
                        toVibrate.SendHapticImpulse(intensity, duration + 0.1f);
                    }
                    else
                    {
                        // ensure the vibrations stop
                        StopHapticFeedback();
                        // toVibrate.SendHapticImpulse(0, 0);
                    }
                }
                yield return new WaitForSeconds(duration);
            }
            else
            {
                if (_activeObject != null)
                {
                    float value = _activeObject.getInteractionValue();
                    if (_sceneSettings.activeControllerHaptics == HapticRenderingFunction.CUBIC)
                    {
                        value = CubicMapping(value);
                    }
                    string state = _isGrabbed ? "1" : "0";
                    string message = "";
                    if (_sceneSettings.activeSceneName == SceneName.BOXES_WEIGHT)
                    {
                        message = $"{_sceneSettings.GetSceneIdentifier()},{(int)_sceneSettings.activeAlgorithm},{state},{_activeObject.getMultiplier()},{value}";
                    }
                    else
                    {
                        message = $"{_sceneSettings.GetSceneIdentifier()},{(int)_sceneSettings.activeAlgorithm},{state},{value}";
                    }

                    if (_sceneSettings.debug)
                    {
                        Debug.Log($"SendData: {message}");
                    }
                    if (_serialCommunicator != null)
                    {
                        _serialCommunicator.SendData(message);
                    }
                }
                yield return new WaitForSeconds(loopDuration);
            }
            // yield return new WaitForSeconds(0.1f);
            }
        }

    }

    void StopHapticFeedback()
    {
        if (_playerRightController != null)
            _playerRightController.SendHapticImpulse(0, 0);
        if (_playerLeftController != null)
            _playerLeftController.SendHapticImpulse(0, 0);
    }

    public void sendReset()
    {
        Debug.Log("sendReset()");
        sceneUnloaded = true;
        if (_sceneSettings.activeAlgorithm == ActuationAlgorithm.CONTROLLER)
        {
            StopHapticFeedback();
        }
        else
        {
            string message = "";
            if (_sceneSettings.activeSceneName == SceneName.BOXES_WEIGHT)
            {
                message = $"{_sceneSettings.GetSceneIdentifier()},{(int)_sceneSettings.activeAlgorithm},{0},{0},{0}";
            }
            else
            {
                message = $"{_sceneSettings.GetSceneIdentifier()},{(int)_sceneSettings.activeAlgorithm},{0},{0}";
            }
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





    // public void sendObjectMessage(ISerialObject activeObject, bool isGrabbed)
    // {
    //     if (activeObject != null)
    //     {
    //         if (SceneSettings.Instance.activeAlgorithm == ActuationAlgorithm.CONTROLLER)
    //         {
    //             HapticImpulsePlayer toVibrate = _playerRightController;
    //             // Actuate controller
    //             if (SceneSettings.Instance.activeMonitoredHand == InteractorHandedness.Left)
    //             {
    //                 toVibrate = _playerLeftController;
    //             }
    //             if (isGrabbed)
    //             {
    //                 if (SceneSettings.Instance.debug)
    //                 {
    //                     Debug.Log($"Starting Controller Vibrations");
    //                 }
    //                 // calculate amplitude
    //                 float amplitude = SceneSettings.Instance.baseAmplitude;
    //                 float objectValue = activeObject.getInteractionValue();
    //                 float intensity = CubicMapping(objectValue) * amplitude;
    //                 float duration = SceneSettings.Instance.hapticDuration;

    //                 hapticCoroutine = StartCoroutine(StartHapticFeedback(toVibrate, intensity, duration));
    //             }
    //             else
    //             {
    //                 if (SceneSettings.Instance.debug)
    //                 {
    //                     Debug.Log($"Stopping Controller Vibrations");
    //                 }
    //                 if (hapticCoroutine != null)
    //                 {
    //                     StopCoroutine(hapticCoroutine);
    //                     hapticCoroutine = null;
    //                 }
    //                 // ensure the vibrations stop
    //                 toVibrate.SendHapticImpulse(0, 0);
    //             }
    //         }
    //         else
    //         {
    //             string state = isGrabbed ? "1" : "0";
    //             string message = $"{SceneSettings.Instance.GetSceneIdentifier()},{SceneSettings.Instance.activeAlgorithm},{state},{activeObject.getInteractionValue()}";
    //             if (_serialCommunicator != null)
    //             {
    //                 _serialCommunicator.SendMessage(message);
    //             }
    //         }
    //     }
    //     else
    //     {
    //         if (SceneSettings.Instance.debug)
    //         {
    //             Debug.LogError($"sendObjectMessage: activeObject == null");
    //         }
    //     }
    // }




    // IEnumerator StartHapticFeedback(HapticImpulsePlayer controller, float amplitude, float duration)
    // {
    //     if (SceneSettings.Instance.debug)
    //     {
    //         Debug.Log($"StartHapticFeedback: {controller} - {amplitude} - {duration}");
    //     }
    //     while (true)
    //     {
    //         controller.SendHapticImpulse(amplitude, duration);
    //         yield return new WaitForSeconds(duration);
    //     }
    // }

    float CubicMapping(float input)
    {
        // Ensure input is within the range [0, 1]
        input = Mathf.Clamp01(input);

        // Apply the cubic mapping
        float output = Mathf.Pow(input, 3);

        return output;
    }

}
