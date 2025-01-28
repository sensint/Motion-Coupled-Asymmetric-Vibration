using System;
using System.IO.Ports;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class SerialCommunicator : MonoBehaviour
{
    private SceneSettings _sceneSettings;

    private string _portName = "COM3";

    private int _baudRate = 9600;

    private SerialPort _serialPort;


    // private bool _debug = false;
    // private bool _fakeConnection = false;

    void Awake()
    {
        _sceneSettings = GameObject.Find("Scene Settings").GetComponent<SceneSettings>();
    }

    void Start()
    {

        // _debug = SceneSettings.Instance.debug;
        // _fakeConnection = SceneSettings.Instance.fakeConnection;

        _portName = _sceneSettings.portName;
        _baudRate = _sceneSettings.baudRate;
        _serialPort = new SerialPort(_portName, _baudRate);
        OpenConnection();
    }

    void Update()
    {

    }

    void OpenConnection()
    {
        if (_sceneSettings.debug && _sceneSettings.fakeConnection)
        {
            Debug.Log($"Faking serial connection on {_portName} with {_baudRate} baud");
            return;
        }
        try
        {
            if (!_serialPort.IsOpen)
            {
                _serialPort.Open();
                if (_sceneSettings.debug)
                {
                    Debug.Log($"Serial port opened on {_portName} with {_baudRate} baud");
                }
            }
        }
        catch (Exception e)
        {
            Debug.LogError("Error opening serial port: " + e.Message);
        }
    }

    public void SendData(string message)
    {
        if (_sceneSettings.debug && _sceneSettings.fakeConnection)
        {
            Debug.Log($"Faking SendData: {message}");
            return;
        }
        if (_serialPort.IsOpen)
        {
            _serialPort.WriteLine(message);
            if (_sceneSettings.debug)
            {
                Debug.Log("Sent: " + message);
            }
        }
        else
        {
            Debug.LogError("Serial port is not open");
        }
    }

    void OnApplicationQuit()
    {
        if (_sceneSettings.debug && _sceneSettings.fakeConnection)
        {
            Debug.Log($"Faking close connection on {_portName} with {_baudRate} baud");
            return;
        }
        if (_serialPort != null && _serialPort.IsOpen)
        {
            _serialPort.Close();
            if (_sceneSettings.debug)
            {
                Debug.Log("Serial port closed");
            }
        }
    }
}
