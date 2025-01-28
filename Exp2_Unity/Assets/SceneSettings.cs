using System.Collections;
using System.Collections.Generic;
using UnityCommunity.UnitySingleton;
using UnityEngine;
using UnityEngine.XR.Interaction.Toolkit.Interactors;

public enum HapticRenderingFunction
{
    LINEAR,
    CUBIC
}

public enum ActuationAlgorithm {
    CONTROLLER = 0,
    CONTINUOUS_PSEUDO_FORCES = 1,
    MOTION_COUPLED_PSEUDO_FORCES = 2,
    CUSTOM = 3
}

public enum SceneName {
    BOW_ARROW = 0,
    BOXES_WEIGHT = 1,
    MAGNETS = 2
}

public class SceneSettings : MonoBehaviour
{
    [Header("Scene Settings")]
    public SceneName activeSceneName = SceneName.BOXES_WEIGHT;

    public ActuationAlgorithm activeAlgorithm = ActuationAlgorithm.CONTROLLER;

    public InteractorHandedness activeMonitoredHand = InteractorHandedness.Right;

    public HapticRenderingFunction activeControllerHaptics = HapticRenderingFunction.LINEAR;

    public float baseAmplitude = 1.0f;
    public float hapticDuration = 0.1f;

    [Header("Serial Connection Settings")]
    public string portName = "COM3";

    public int baudRate = 9600;

    public float loopDuration = 0.01f;

    [Header("Debug Settings")]
    public bool debug = false;
    public bool fakeConnection = false;

    void Update()
    {

    }

    public string GetSceneIdentifier()
    {
        return sceneNameToString(this.activeSceneName);
    }

    public string sceneNameToString(SceneName name)
    {
        switch(name)
        {
            case SceneName.BOW_ARROW:
            return "B";
            case SceneName.BOXES_WEIGHT:
            return "W";
            case SceneName.MAGNETS:
            return "M";
        }
        return "X";
    }
}
