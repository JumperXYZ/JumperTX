                                 
                                    
## JumperTX is firmware for the Jumper Technology Radios forked from the OpenTX project and independantly maintained by Jumper Technology. 

OpenTX are not affiliated with the JumperTX fork.

Visit us on facebook https://www.facebook.com/groups/2155287984750165/

If you are looking to contribute to the JumperTX project please email rcjumper at hotmail dot com.

Full credit is given to the OpenTX team. Please visit the site below to learn more about OpenTX
http://www.open-tx.org

Compile： cmake -DPCB=T16 -DGUI=YES -DGVARS=YES -DHELI=YES -DLCD_DUAL_BUFFER=YES  -DLUA=YES -DLUA_COMPILER=YES -DMODULE_R9M_FULLSIZE=YES -DMULTIMODULE=YES -DPPM_CENTER_ADJUSTABLE=YES -DPPM_UNIT=US -DRAS=YES  -DCMAKE_BUILD_TYPE=Release ../

A Docker container for building JumperTX for the Jumper T16 radio with the default or your own build flags is available here: https://hub.docker.com/r/benlye/jumpertx-build
