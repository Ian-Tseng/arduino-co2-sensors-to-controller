#!/usr/bin/env python
# coding: utf-8

# In[ ]:


from Arduino import Arduino
from pySerialTransfer import pySerialTransfer as txfer
import traceback
import serial
import time
import time
import csv
import os
import sys
import argparse


# In[ ]:


def save_data(dir, datas, mode):
    try:
        with open(dir, mode, newline='') as csvfile: 
            if mode== 'w+':
       
                writer = csv.writer(csvfile, delimiter=',')
                writer.writerow(['Time', 'Sensor_1'])
            
            elif mode== 'a+': 
       
                writer = csv.writer(csvfile, delimiter=',')
                writer.writerow(datas)
    except:
        print ("It can't write the data when you open the current running excel data.")
        


if __name__ == '__main__':

    init_time= time.time()
    parser = argparse.ArgumentParser()
    parser.add_argument('--sensor1',

                       default= 7, #7

                       help='input the number of sensor1 port')

    args = parser.parse_args()
    date_time= time.ctime().replace(' ', '_')
    date_time= date_time.replace(':', '_')
    data_dir= os.path.join(os.getcwd(), f'sensor_data_{date_time}.csv')

  

    try:
        sensor0_port= 'COM'+ str(args.sensor1)

     

        link_0 = txfer.SerialTransfer(sensor0_port, timeout=10)
        links_list= [link_0]
        [i.open() for i in links_list]

        

        time.sleep(5)

        
        sensor_0_value= None


        ports_opened= True
        if not link_0.available():
            print (f'Port do not open.')
            ports_opened=False

        else:
           
            save_data(data_dir, None, 'w+')
            print (f'Save data: sensor_data_{date_time}.csv. Do not open it when running program otherwise it may cause program to stop.')


        while ports_opened:

            
            #try:                     

                if link_0.available():
   
                    sensor_0_value = link_0.rx_obj(obj_type='f')
                    if 0<sensor_0_value<= 0.2:
                        print ('Got an outlier for testing:', sensor_0_value)
                        continue

                    datas= [time.ctime(), round(sensor_0_value, 3)]
                    c_time, sensor_1_v= time.ctime(), round(sensor_0_value, 3)
                    print ('Time: ' ,c_time, ', sensor1: ', sensor_1_v )
                    save_data(data_dir, datas,'a+')
            

                elif link_0.status < 0 :

                    if link_0.status == txfer.CRC_ERROR:
                        print('ERROR: CRC_ERROR')
                    elif link_0.status == txfer.PAYLOAD_ERROR:
                        print('ERROR: PAYLOAD_ERROR')
                    elif link_0.status == txfer.STOP_BYTE_ERROR:
                        print('ERROR: STOP_BYTE_ERROR')
                    else:
                        print('ERROR: {}'.format(link_0.status))



           

        

    except KeyboardInterrupt:

        [i.close() for i in links_list]

        print('Interrupt manually.')
        
        


# link_0 = txfer.SerialTransfer('COM7')
# 
#         
# link_0.open()
# time.sleep(5)
#         
# sensor_0_value= None
# sensor_1_value= None
#     
# if __name__ == '__main__':
#         try:
#             while True:
#             
#                 
#             
#                 if link_0.available():
#                     val=  link_0.rx_obj(obj_type='f')
#                     print (val)
#                 elif link_0.status < 0:
#                     if link.status == txfer.CRC_ERROR:
#                         print('ERROR: CRC_ERROR')
#                     elif link.status == txfer.PAYLOAD_ERROR:
#                         print('ERROR: PAYLOAD_ERROR')
#                     elif link.status == txfer.STOP_BYTE_ERROR:
#                         print('ERROR: STOP_BYTE_ERROR')
#                     else:
#                         print('ERROR: {}'.format(link.status))
#            
#                     
#        
#         except KeyboardInterrupt:
#                     try:
#                         link.close()
#                       
#                     except:
#                         pass
#                     print('Interrupt manually.')
#         except :
#             link_0.close()
#             
#         

# In[ ]:




