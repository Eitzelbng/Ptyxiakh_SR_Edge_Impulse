import struct
import re

input_file = "s.txt" 
output_file = "pico_audio_rescued.bin"
count = 0

with open(input_file, "r") as f_in, open(output_file, "wb") as f_out:
    for line in f_in:
        # Use RegEx to find all integers (including negative ones) in the line
        # This turns "91-21" into ["91", "-21"]
        found_values = re.findall(r'-?\d+', line)
        
        for val_str in found_values:
            try:
                value = int(val_str)
                # Clamp to 16-bit range
                #value = max(-2147483647, min(2147483647, value))
                value = max(-32768, min(32767, value))
                #f_out.write(struct.pack('<i', value))
                f_out.write(struct.pack('<h', value))
                count += 1
            except ValueError:
                continue

print(f"Rescued {count} samples into {output_file}")