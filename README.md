# Mini Search Engine

A C-based mini search engine that indexes text documents and retrieves relevant results using an inverted index and hash-based lookup.

## Overview

This project implements the core components of a simple search engine in C. It processes text documents, builds an inverted index for keyword lookup, and provides a graphical interface for searching indexed content.

The project was developed as a team project.

## Features

- Text document indexing
- Inverted index for keyword-based search
- Hash table for efficient lookup
- Text preprocessing and cleaning
- Search across multiple text files
- Graphical user interface using GTK
- Ranking of search results

## Project Structure

```text
Mini-Search-Engine/
│
├── files/
│   ├── AI_1.txt
│   ├── AI_2.txt
│   ├── ...
│   └── AI_10.txt
│
├── gui.c
├── hash.c
├── hash.h
├── inverted.c
├── inverted.h
├── main.c
├── search_engine.c
├── search_engine.h
├── text_processor.c
├── text_processor.h
└── .gitignore
```

## How It Works

```text
Text Documents
      ↓
Text Preprocessing
      ↓
Tokenization / Cleaning
      ↓
Hash Table
      ↓
Inverted Index
      ↓
Search Query
      ↓
Result Ranking
      ↓
Search Results
```

## Technologies

- C
- GTK
- Hash Tables
- Inverted Index
- Text Processing
- Data Structures

## Team Project

Developed collaboratively as a three-member C programming project.

## Contributors

- Arshita Gupta
- Animesh Gupta
- Divanshi Mehta