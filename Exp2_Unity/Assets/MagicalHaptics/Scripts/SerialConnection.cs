using System;
using System.IO.Ports;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class SerialConnection : MonoBehaviour
{
    private SerialPort serialPort;
    public string portName = "COM3"; // Change this to your specific port name
    public int baudRate = 9600;

    [SerializeField]
    public GameObject _gameObject;

    public ISerialObject _serialObject;

    void Start()
    {
        _serialObject = _gameObject.GetComponent<ISerialObject>();
        if (_serialObject == null)
        {
            Debug.Log("_serialObject == null");
        }
        // Initialize the serial port
        serialPort = new SerialPort(portName, baudRate);
        OpenConnection();
    }

    void Update()
    {
        // if (_serialObject != null && _serialObject.hasMessage())
        // {
        //     Debug.Log(_serialObject.composeMessage());
        // }

        if (serialPort.IsOpen)
        {
            try
            {
                //string message = "";
                if (_serialObject != null && _serialObject.hasMessage())
                {
                    SendData(_serialObject.composeMessage());
                }
                // // Example of sending a vector direction to Teensy
                // Vector3 direction = new Vector3(1, 0, 0); // Example vector, replace with your actual vector
                // string messageToSend = $"{direction.x},{direction.y},{direction.z}\n";
                // SendData(messageToSend);

                // Optionally, read data from Teensy
                if (serialPort.BytesToRead > 0)
                {
                    string dataFromTeensy = serialPort.ReadLine();
                    Debug.Log("Received: " + dataFromTeensy);
                }
            }
            catch (Exception e)
            {
                Debug.LogError("Error: " + e.Message);
            }
        }
    }

    void OpenConnection()
    {
        try
        {
            if (!serialPort.IsOpen)
            {
                serialPort.Open();
                Debug.Log("Serial port opened");
            }
        }
        catch (Exception e)
        {
            Debug.LogError("Error opening serial port: " + e.Message);
        }
    }

    void SendData(string message)
    {
        if (serialPort.IsOpen)
        {
            serialPort.WriteLine(message);
            Debug.Log("Sent: " + message);
        }
        else
        {
            Debug.LogError("Serial port is not open");
        }
    }

    void OnApplicationQuit()
    {
        if (serialPort != null && serialPort.IsOpen)
        {
            serialPort.Close();
            Debug.Log("Serial port closed");
        }
    }
}
