# Amplification-Free System for High-Resolution Josephson Junction Characterization

# Raspberry Pi + HifiBerry setup
The Raspberry Pi will contain the server-side programs. 


### To run an experiment

- Note that all the codes now are in the "I/Odata-hifiberry" branch, not the main one
- download programs in src/client (they are necessary to control the card and perform the experiment)
- experiment.py is the old code we used for the acquistions in the university lab
- experiment_new.py is the new code that allows for an arbitrary number of averages
- download "experiment.ipynb" in examples (to control the experiment)

Inside experiment.ipynb
- set the parameters as you like
- create a folder to save the data in and specify its path in folder_path
