# Bank Buster
# Global Currency Exchange Network

## Overview  
This project is a C++ based simulation of a global banking and currency exchange network. It models banks as nodes and currency exchanges as edges using graph data structures, supporting multiple financial operations and analysis.

## Features  
- Currency conversion using shortest path logic  
- Maximum profit path calculation  
- Arbitrage detection using Bellman-Ford  
- Large volume transfer using Max Flow  
- Bank collapse simulation using Tarjan SCC  
- Bank rate rankings using BST  
- Admin control to modify network  

## Workflow  
Banks are represented as nodes and currency exchanges as edges with rate, fee, and capacity.  

Users can perform conversions, detect arbitrage opportunities, and analyze optimal paths.  

The system evaluates exchange paths considering fees and rates to maximize output.  

Network stability can be tested by simulating bank removal and observing connectivity changes.  

## Tech Stack  
- C++  
- Graph (Adjacency List)  
- BST  
- Stack  
- File handling using CSV  

## Structure  
- currency_exchange_network.cpp: main logic  
- bank_network.csv: stored network data  

## Compilation  
g++ currency_exchange_network.cpp -o exchange  

## Usage  
./exchange  

## Notes  
The project demonstrates real-world financial network modeling using core data structures and algorithms.
