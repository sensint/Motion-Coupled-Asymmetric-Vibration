using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

public class SceneTimer : MonoBehaviour
{
    private SceneSettings _sceneSettings;

    public bool enableTimer = false;

    public float minutesUntilSceneChange = 1.0f;

    public string newScene = "";

    [SerializeField]
    public GameObject hapticManager;

    [SerializeField]
    private float timePassed;


    // private float startTime;

    void Awake()
    {
        _sceneSettings = GameObject.Find("Scene Settings").GetComponent<SceneSettings>();
    }


    // Start is called before the first frame update
    void Start()
    {
    }

    // Update is called once per frame
    void Update()
    {
        if (enableTimer)
        {
            // float timePassed = Time.time - startTime;
            timePassed = Time.time;
            if (_sceneSettings.debug)
            {
                Debug.Log(timePassed + " - " + (minutesUntilSceneChange * 60.0f));
            }
            if (timePassed > (minutesUntilSceneChange * 60.0f))
            {
                try
                {
                    if (hapticManager != null)
                    {
                        IHapticManager mng = hapticManager.GetComponent<IHapticManager>();
                        if (mng != null)
                        {
                            mng.sendReset();
                        }
                    }

                    SceneManager.LoadScene(newScene);
                }
                catch (Exception e)
                {
                    Debug.LogException(e);
                }
            }
        }
    }
}
