## ⚖️ License & Disclaimer

Copyright (c) 2026 

The original source code developed specifically for this graduation thesis project is licensed under the **MIT License**:

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

---

## 🛠️ Third-Party Software, Libraries & Data Credits

This project incorporates and builds upon the following open-source software, libraries, and datasets. Each component remains subject to its respective author's license terms:

* **[Edge Impulse Generated Library](https://edgeimpulse.com)**: The Machine Learning model pipeline and digital signal processing/feature extraction library components were trained and deployed via the Edge Impulse platform. This generated deployment code is integrated under the terms of the **Apache License 2.0**.
* **[ESC-50 Dataset](https://github.com)**: The Environmental Sound Classification dataset by Karol J. Piczak was utilized for training and evaluating the audio classification algorithms. This dataset is included under the **Creative Commons Attribution Non-Commercial license (CC BY-NC)**.
* **[pico-lora Library](https://github.com/akshayabali/pico-lora)**: The LoRa communication driver library developed by @akshayabali for the Raspberry Pi RP2040 microcontroller is embedded in this project. It is integrated under the terms of the **MIT License**.


## Instructions on Importing Custom Edge-Impulse library export

To create your own custom Edge Impulse library export please refer to official guides made by Edge Impulse.

After building your own custom Edge Impulse library you can import it by replacing 

A) The contents of the Transit/Libraries/Edge_Impulse with the extracted contents of your own Edge model.
B) All instances of tflite_learn_794595_164. In the main Transmit.cpp and CMakeLists.txt to your own generated tflite namefile (you can find it on your exported file in Edge_Impulse/tflite-model/(here)).
C) Note in Main code line 304 , In this example result.classification[5].value represents silence. Pseudo-logic code was created as a filter for the recognition of undefined noisy background. Removing this segment and implementing your own logic based on your own needs is recommended.
D) Pio synchronization at 16khz was achieved via trial and error through constant measurements via an oscilloscope, static values on Transmitter.cpp line 162 function (pio_sm_set_clkdiv_int_frac(pio0, STATE_MACHINE, 18, 79);) do not guarantee 16khz tick in all applications, altering the constant 18 and 79 values might be necessary).
