import GPUtil

def get_gpu_temperature():
    gpus = GPUtil.getGPUs()
    gpu_temperature = gpus[0].temperature
    return gpu_temperature

while True:
    print(get_gpu_temperature())