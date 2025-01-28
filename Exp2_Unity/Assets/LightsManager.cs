using System.Collections;
using System.Collections.Generic;
using Unity.VRTemplate;
using UnityEngine;

public class LightsManager : MonoBehaviour
{
    bool isItDark = false;

    public GameObject plane = null;

    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        if (Input.GetKeyDown(KeyCode.L))
        {
            this.toggleLights();
        }
    }

    void toggleLights()
    {
        if (plane != null)
        {
            if (isItDark)
            {
                // disable
                plane.SetActive(false);
                isItDark = false;
            }
            else
            {
                // enable
                plane.SetActive(true);
                isItDark = true;
            }
        }
    }
}
