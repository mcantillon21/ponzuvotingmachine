# The Ponzu Voting Terminal: CS107e Final Project (Molly and Pun)

**TLDR**

A fraud proof voting machine for the people. 

Problem: We need a new lecturer. We must choose between existing legend Christos Kozyrakis and supreme all-knowing overlord Matthew Trost. 

How can we ensure that our singular casted vote is counted while maintaining complete anonymity to other voters?

Our proposal uses Merkle Trees and cryptographic hashing functions to verify a user's vote while preserving their anonymity. An accompanying voting frontend authenticates users to vote once, and only once, by their keystroke biometrics. 

## Roles:
Pun: 
- Designing cryptographic system
- Voting implementation
  - Merkle Tree Implementation
  - Vote Tickets
- Fraud Proofs Implementation
  - Merkle Tree Computation
  - Vote certification system
- Implementation and design
  - Merkle/Fraud Proof Page
  - Certification Request Page
Molly: 
- Designing keystroke authentication system
- Keystroke Authentication (editing keyboard drivers/interrupts)
- Implementation and design
  - Vote Page
  - Home Page
  - Admin/Admin Auth Page
  - Auth Page

**Description:**
Many say that the results of an election are determined by the counter. To challenge this, we create a fraud proof voting system that satisfies the following properties:
Voters are guaranteed that their votes are counted
Voter Privacy holds (one voter doesn’t know who another voter chose)
Voters are guaranteed that only the votes of some N voters are counted if voters don’t collude with the counter.

We therefore introduce the Ponzu voting machine to fix such a problem. In order for the Ponzu voting machine to be secure, we require that all N voters are aware that there are only N voters, and are all aware of one another’s identities. Moreover, we require that for the election to be fraud-free, all N voters exert their rights to produce a fraud proof.

## How does the fraud proof work?: 
Simply put, after each vote, the central authority will hash the vote object, and digitally sign it, as a ‘promise’ to the voters that they’ve included the vote in the election. After the election, the central authority will construct a Merkle Tree from all the votes counted in the election, and publish the Merkle Root Node which contains the outcome of the election. After the election, each voter who holds a certificate, and is thus ‘promised’ to be included in the election is able to request a fraud proof from the server - if the server counted the votes honestly, then they will be able to generate a ‘merkle path’ which is basically a sequence of data which, when hashed with the user’s vote, will output the merkle root. Because the server cannot computationally do this unless the user’s vote is included in the merkle tree, it proves to users that their vote is included. If the server refuses to provide a fraud proof or provides an invalid one, the voters can share their certificate to other voters, who would try to query the server. If the server still refuses to provide a fraud proof, then the election’s credibility is lost.

## How does the keystroke authentication work?:

To register to cast a vote, each voter will type a ‘secret’ vote phrase to populate our database. While doing this, our keyboard interrupt driver is recording and caching the timing of each keypress – even though there's some latency in updating the screen, the keypress timings, independent of the console, are relatively precise. Using this data, we calculate the periodicity of each key press per letter and store those values into a struct. 

When casting a vote, the user not only has to enter the correct ‘secret’ vote phrase, but they will also need to type in a similar manner as when they registered. This means emulating similar timing intervals to the initial input. We did this by calculating the mean-squared error (MSE) between the key-stroke intervals when registering and voting. If the MSE falls below a certain threshold, and the secret phrase is accurate, the user is authorised to vote.


## More Details on Fraud Proofs:
Beware - we thought of this in a day or so. Only the following 3 properties hold (we think), but there are a bunch of other problems with the system (e.g. the server may collude with voters to cheat etc.)

**Registration:**
All N voters will register a pass-phrase with the central authority (vote counter). All N voters are aware of each others’ identities.

**Voting:**
The user can later exercise their passphrase to the central authority. If the passphrase is valid, the authority will store the votes as follows in the database.

```
struct {
	Hash: H(password)
	Randomness: 1256
Vote: 0 || 1
} vote (this is slightly different from in the code)
```

The authority will then hash the vote to generate a certificate, and then provide a digital signature on the vote - this basically is the authority ‘promising’ that the vote has been counted.

**Creating Merkle Tree:**
Each vote will be hashed into a merkle node as follows. Each of the votes are constructed into the merkle’s leaf nodes, and each non-leaf node is calculated as follows.
```
/// Non Leaf Nodes
struct {
   Hash: H(LeftChild | RightChild)
   Count: LeftChild.count + RightChild.count
} merkleNode

/// Leaf Nodes
struct {
   Hash: H(Vote)
   Count: Vote.count
} merkleNode

Note: These are the same structs.
```


At the end of the election, the authority will publish the final vote count and the hash of the e

**Generating Fraud Proof:**
To request fraud proof, a voter sends their certificate to the authority, who will send a valid merkle proof. There is negligible probability that they are able to fraud such a proof. The voter will then validate a Merkle Proof by checking that each Merkle Node in their path is valid (hashing and vote counts honestly computed). The voter must also check that their votes are in the first N leafs of the tree.

**Proving that no fake votes occurred:**
Every voter will be able to request for the merkle proof of the (N + 1)th leaf of the Merkle Tree. If the server truly only counted N votes, then the merkle proof for this leaf will have a count of 0 for all right neighbours in the path (i.e. any leaf to the right of 0 has a total count of 0)

## Improvements for Future Implementation:
During the course of this project, we sought to build a completely fraud-proof, immutable voting machine aided by the security of keystroke authentication and cryptographic merkel trees. We achieved this aspiration despite a few minor inaccuracies which do not affect the core functionality of the machine. 

In future implementations, we hope to ameliorate the process of sampling and processing keystroke data by requiring secret vote phrases of a minimum length and comparing against a larger dataset. We may also consider migrating to a better-suited coding language for cryptography (e.g. Rust), so that we can utilize mature cryptographic libraries with newer primitives like zero knowledge proofs, instead of bare-metal RPi programming. Lastly, we intend to add an element of networking that further strengthens the security of our machine by separating server and vote-casting client. 

## Credits:
The voting merkle tree was inspired by the Proof of Reserve solution used by cryptocurrency exchanges to prove to their users that their assets are actually being stored in their reserves. (https://niccarter.info/proof-of-reserves/)

We used SHA-256 code implemented by Brad Conte. (https://github.com/B-Con/crypto-algorithms/blob/master/sha256.c)


