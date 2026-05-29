import pandas as pd

# Load the file
df = pd.read_csv('Games.csv')

# 1. Replace newlines with spaces in all object columns (text fields)
# This prevents the parser from thinking a new line is a new record
for col in df.select_dtypes(include=['object']):
    df[col] = df[col].replace(r'\n|\r', ' ', regex=True)

# 2. Optionally, escape or remove quotes if they are causing issues
# A simple way to avoid quote-parsing errors is to remove them or replace with single quotes
for col in df.select_dtypes(include=['object']):
    df[col] = df[col].str.replace('"', "'", regex=False)

# Save the cleaned file
df.to_csv('Games_Cleaned.csv', index=False, quoting=1) # quoting=1 ensures all fields are quoted, which is safer
print("File cleaned and saved as Games_Cleaned.csv")
