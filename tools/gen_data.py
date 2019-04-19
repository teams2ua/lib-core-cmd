from pycoin.key.BIP32Node import BIP32Node
from blockchain.datas import *
import hashlib
import random


def random_sha():
    return hashlib.sha256(bytearray(str(random.random()), 'utf-8')).hexdigest()


# commands is in the form ("spend", X) or ("get", X)
# get is always done from some random outputs
# spend is simulating UTXO picking
# each operation live in its own block

def generate(root, commands, fee):
    external = root.subkey(0)
    internal = root.subkey(1)
    cur_external = 0
    cur_change = 0
    # receive number_of_donations transactions
    UTXOs = []
    prevBlock = Block(0,
                      "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f",
                      "0000000000000000000000000000000000000000000000000000000000000000",
                      [])
    yield prevBlock
    for (commandType, amount), op_index in zip(commands, itertools.count()):
        inps = []
        outs = []
        if commandType == "get":
            giverUTXO = TransactionOutput(root.address(), amount + fee)
            giverUTXO.transaction_hash = random_sha()
            giverUTXO.index = 0
            inps.append(TransactionInput(giverUTXO))
            out = TransactionOutput(external.subkey(cur_external).address(), amount)
            cur_external += 1
            UTXOs.append(out)
            outs.append(out)
        elif commandType == "send":
            totalSum = 0
            while UTXOs:
                utxo = UTXOs[0]
                UTXOs.pop(0)
                totalSum += utxo.value
                inps.append(TransactionInput(utxo))
                if totalSum >= fee + amount:
                    break
            if totalSum < fee + amount:
                raise ValueError("Not enough fund to make {0}-th operation {1}", op_index, (commandType, amount))
            outs.append(TransactionOutput(root.address(), amount))
            rest = totalSum - fee - amount
            if rest > 0:
                our_out = TransactionOutput(internal.subkey(cur_change).address(), rest)
                cur_change += 1
                outs.append(our_out)
                UTXOs.append(our_out)
        transaction = Transaction(random_sha(), inps, outs)
        newBlock = Block(prevBlock.header.height + 1, random_sha(), prevBlock.header.hash, [transaction])
        yield newBlock
        prevBlock = newBlock
    return


if __name__ == '__main__':
    root = BIP32Node.from_master_secret(b'ledger')  # account node
    print(root)
    commands = []
    for i in range(1000):
        commands.append(("get", 1001))
    for i in range(10000):
        commands.append(("send", 100))
    bkch = [x for x in generate(root, commands, 0)]
    serialize_to_pickle_file(Blockchain(bkch), "data.pkl")
    #write_to_json_file(bkch)
