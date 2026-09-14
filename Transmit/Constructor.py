import os
import shutil

# --- CONFIG ---
source_dir = "C:/Users/Angelos/Documents/Ptyxiakh/SoundEffects/audio"
output_dir = "Sorted_ESC50_Samples"

# Map the class ID (the last number in the filename) to the name
class_map = {
    "10": "rain",
    "14": "birds",
    "20": "baby",
    "42": "engine",
    "43": "siren",
    "46": "bells"
}

# Create the output root
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

print("Sorting files by filename pattern...")

count = 0
for filename in os.listdir(source_dir):
    if filename.endswith(".wav"):
        # Split '1-12654-A-15.wav' by the hyphens
        parts = filename.split('-')
        
        if len(parts) == 4:
            # Get the ID (strip the .wav extension)
            class_id = parts[3].replace(".wav", "")
            
            # If it's one of our target classes, move it
            if class_id in class_map:
                label = class_map[class_id]
                target_folder = os.path.join(output_dir, label)
                
                # Create the specific class folder if it doesn't exist
                os.makedirs(target_folder, exist_ok=True)
                
                # Copy the file
                shutil.copy2(os.path.join(source_dir, filename), os.path.join(target_folder, filename))
                count += 1

print(f"/nSuccess! Sorted {count} files into '{output_dir}'.")