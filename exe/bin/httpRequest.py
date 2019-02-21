import requests
import sys

def usage():
    print ('usage: python ', sys.argv[0], ' [command] [parameters]')
    print ('command:')
    print ('    uploadSensorData')
    print ('    getSensorData')
    print ('    uploadModuleCheckResult')
    print ('    bindMachineSensor')
    print ('    uploadCameraCheckResult')
    print ('    getSensorsData')
    return

def uploadSensorData ():
    if (len(sys.argv) != 6 and len(sys.argv) != 5) :
        print ('usage: python ', sys.argv[0], ' uploadSensorData uuid sensor_id max_value vig_path ')
        print ('    or python ', sys.argv[0], ' uploadSensorData uuid sensor_id bpc_path')
        return -1

    uuid = sys.argv[2]
    sensor_id = sys.argv[3]

    url = 'http://air.produce.arashivision.com/pro/sensor/uploadSensorData'
    if (len(sys.argv) == 6) :
        max_value = sys.argv[4]
        vig_path = sys.argv[5]
        data = {
                'uuid': uuid,
                'sensor_id': sensor_id,
                'max_value': max_value,
                }
        files = [
                ('vig', (vig_path, open(vig_path, 'rb')))
                ]
    else :
        bpc_path = sys.argv[4]
        data = {
                'uuid': uuid,
                'sensor_id': sensor_id,
                }
        files = [
                ('bpc', (bpc_path, open(bpc_path, 'rb')))
                ]

    r = requests.post(url, params=data, files=files)
    print (r.url)
    print (r.text)
    return

def getSensorData ():
    if (len(sys.argv) != 4) :
        print ('usage: python ', sys.argv[0], ' getSensorData uuid sensor_id')
        return -1

    uuid = sys.argv[2]
    sensor_id = sys.argv[3]

    url = 'http://air.produce.arashivision.com/pro/sensor/getSensorData'
    param = {
            'uuid': uuid,
            'sensor_id': sensor_id
            }

    header = {'content-type': 'x-www-form-urlencoded'}

    r = requests.post(url, params=param, headers=header)
    print (r.url)
    print (r.text)
    return

def uploadModuleCheckResult():
    if (len(sys.argv) != 5) :
        print ('usage: python ', sys.argv[0], ' uploadModuleCheckResult uuid sensor_id ok')
        return -1

    uuid = sys.argv[2]
    sensor_id = sys.argv[3]
    ok = sys.argv[4]

    url = 'http://air.produce.arashivision.com/pro/sensor/uploadCheckResult'
    param = {
            'uuid': uuid,
            'sensor_id': sensor_id,
            'ok': (ok != '0')
            }

    header = {'content-type': 'x-www-form-urlencoded'}

    r = requests.post(url, params=param, headers=header)
    print (r.url)
    print (r.text)
    return

def bindMachineSensor():
    if (len(sys.argv) != 3) :
        print ('usage: python ', sys.argv[0], ' bindMachineSensor serial_sensors_json')
        return -1

    json = argv[2]

    url = 'http://air.produce.arashivision.com/pro/machine/bindMachineSensor'

    header = {'content-type': 'application/json'}

    r = requests.post(url, json, headers=header)
    print (r.url)
    print (r.text)
    return

def uploadCameraCheckResult():
    if (len(sys.argv) != 4) :
        print ('usage: python ', sys.argv[0], ' uploadCameraCheckResult serial ok')
        return -1

    serial = sys.argv[2]
    ok = sys.argv[3]

    url = 'http://air.produce.arashivision.com/pro/sensor/uploadCheckResult'
    param = {
            'serial': serial,
            'ok': (ok != '0')
            }

    header = {'content-type': 'x-www-form-urlencoded'}

    r = requests.post(url, params=param, headers=header)
    print (r.url)
    print (r.text)
    return

def getSensorsData():
    if (len(sys.argv) != 3) :
        print ('usage: python ', sys.argv[0], ' getSensorsData sensors_json')
        return -1

    json = sys.argv[2]
    print (json)

    url = 'http://air.produce.arashivision.com/pro/sensor/getSensorsData'

    header = {'content-type': 'application/json'}

    r = requests.post(url, json, headers=header)
    print (r.url)
    print (r.text)
    return

#main
if (len(sys.argv) < 2) :
    usage()
    sys.exit(-1)

api = sys.argv[1]

res = 0
if (api == 'uploadSensorData') :
    res = uploadSensorData()
elif (api == 'getSensorData') :
    res = getSensorData()
elif (api == 'uploadModuleCheckResult') :
    res = uploadModuleCheckResult()
elif (api == 'bindMachineSensor') :
    res = bindMachineSensor()
elif (api == 'uploadCameraCheckResult') :
    res = uploadCameraCheckResult()
elif (api == 'getSensorsData') :
    res = getSensorsData()
else :
    usage()

sys.exit(res)
