# B-L4675-IOT01A-Voice-Keyword-Recognition

This code is written to be deployed into the STM32 IoT Discovery Board (P/N: B-L475-IOT01A). 

Uploading the .bin file to your IoT Discovery Board will allow the board to use the onboard PDM microphone to recognize 8 different keywords:

- Up
- Down
- Stop
- Go
- Yes
- No
- Left
- Right

Only one of these words will be outputted when spoken, and I abritratily decided that 60% likelihood of being correct was good enough to cutoff all the uncertainty.

If a word is spoken but it does not fit into those 8 categories, it is supposed to be classified as "unknown". This currently doesn't work that well because I am using the free version of Edge Impulse, and so I limited the total data to each category (including "unknown") to 20 mins of data each.

The data pushed to Edge Impulse originates from the Google public voice datasets. The full code that pushes this data to the Edge Impulse application is shown in the "Labelled_Custom_audio_dataset_curation.ipynb" file, it might need some modification if you decide to push similar data to your own implementation of Edge Impulse.

The link to the Edge Impulse ML project is: https://studio.edgeimpulse.com/public/83416/latest

PS. Don't forget to unzip the Edge-Impulse-SDK files :)


