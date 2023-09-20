import csv
import os

# Define the input CSV file and the directory to store the split files
input_csv_file = '../data/datasets/Ericsson/cpu.csv'
output_directory = 'datasets/cpu-splitted'

if not os.path.exists(output_directory):
    os.makedirs(output_directory)

# Function to split the CSV file into equal parts
def split_csv_into_parts(input_csv_file, output_directory, num_parts=40):
    with open(input_csv_file, 'r', newline='') as csvfile:
        csvreader = csv.reader(csvfile)
        data = list(csvreader)

    total_rows = len(data)
    rows_per_part = total_rows // num_parts

    for i in range(num_parts):
        start_idx = i * rows_per_part
        end_idx = (i + 1) * rows_per_part if i < num_parts - 1 else total_rows
        chunk_data = data[start_idx:end_idx]

        output_file = os.path.join(output_directory, f'part_{i + 1}.csv')
        with open(output_file, 'w', newline='') as output_csvfile:
            csvwriter = csv.writer(output_csvfile)
            for row in chunk_data:
                for item in row:
                    csvwriter.writerow([item])  # Write each item in a separate row

if __name__ == "__main__":
    split_csv_into_parts(input_csv_file, output_directory)
