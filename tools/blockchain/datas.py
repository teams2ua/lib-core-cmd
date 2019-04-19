import itertools
import pickle


class Blockchain:
    def __init__(self, blocks):
        self.blocks = blocks


class BlockHeader:
    def __init__(self, height, hash):
        self.height = height
        self.unixTimeInSec = height * 100
        self.hash = hash

class Block:
    def __init__(self, height, hash, parent_hash, transactions):
        self.transactions = transactions
        self.header = BlockHeader(height, hash)
        self.parent_hash = parent_hash
        for tr in transactions:
            tr.add_to_block(self.header)


class TransactionInput:
    def __init__(self, transaction_output):
        self.transaction_output = transaction_output
        self.index = None

    def add_to_transaction(self, index):
        self.index = index


class TransactionOutput:
    def __init__(self, address, value):
        self.address = address
        self.value = value
        self.index = None
        self.transaction_hash = None

    def add_to_transaction(self, index, hash):
        self.index = index
        self.transaction_hash = hash


class Transaction:
    def __init__(self, hash, inputs, outputs):
        self.id = hash
        self.inputs = inputs
        self.outputs = outputs
        self.txHash = hash
        self.unixTimeInSec = None
        self.block_header = None
        self.txHex = hash
        for i, ind in zip(self.inputs, itertools.count()):
            i.add_to_transaction(ind)
        for o, ind in zip(self.outputs, itertools.count()):
            o.add_to_transaction(ind, hash)

    def add_to_block(self, block_header):
        self.block_header = block_header
        self.unixTimeInSec = block_header.unixTimeInSec

    def get_fee(self):
        return sum(inp.transaction_output.value for inp in self.inputs) - self.get_amount()

    def get_amount(self):
        return sum(outp.value for outp in self.outputs)


def serialize_to_pickle_file(blockchain, file_name):
    with open(file_name, "wb") as fl:
        pickle.dump(blockchain, fl)


def deserialize_from_pickle_file(file_name):
    with open(file_name, "rb") as fl:
        return pickle.load(fl)

def input_to_json(tr_input):
    return '["co.ledger.blockchain.commons.models.BitcoinLikeModels$DefaultVin",' \
           '{{' \
           '"outputId":"{0}",' \
           '"outputIndex":{1},' \
           '"value":{2},' \
           '"address":"{3}"' \
           '}}]'.format(random_sha(), 0, 100, tr_input)


def output_to_json(tr_output):
    return '{{' \
           '"outputIndex":{0},' \
           '"value":{1},' \
           '"address":"{2}",' \
           '"scriptHex":"{3}"' \
           '}}'.format(tr_output.index, tr_output.value, tr_output.address, random_sha())


def inputs_to_json(inputs):
    res = '["scala.collection.immutable.$colon$colon",['
    res += ','.join([input_to_json(inp) for inp in inputs])
    return res + ']]'


def outputs_to_json(outputs):
    res = '["scala.collection.immutable.$colon$colon",['
    res += ','.join([output_to_json(o) for o in outputs])
    return res + ']]'


def block_id_to_json(block_id):
    return '{{"height":{0},"hash":"{1}"}}'.format(block_id[1], block_id[0])


def transaction_to_json(tr):
    res = '["co.ledger.blockchain.commons.models.BitcoinLikeModels$BitcoinLikeTransaction",'
    res += '{{' \
           '"id":"{0}",' \
           '"txHash":"{1}",' \
           '"unixTimeInSec":{2},' \
           '"lockTime":{3},' \
           '"fees":{4},' \
           '"blockID":{5},' \
           '"inputs":{6},' \
           '"outputs":{7},' \
           '"txHex":"{8}"' \
           '}}'.format(tr.id,
                       tr.txHash,
                       tr.unixTimeInSec,
                       0,
                       0,
                       block_id_to_json(tr.block_id),
                       inputs_to_json(tr.inputs),
                       outputs_to_json(tr.outputs),
                       tr.txHex)
    return res + ']'


def transactions_to_json(transactions):
    res = '["scala.collection.immutable.$colon$colon",['
    res += ",".join([transaction_to_json(tr) for tr in transactions])
    return res + ']]'


def block_to_json(block):
    return '{{"hash":"{0}",' \
           '"height":{1},' \
           '"unixTimeInSec":{2},' \
           '"parentHash":"{3}",' \
           '"txs":{4}}}\n'.format(block.hash,
                                block.height,
                                block.unixTimeInSec,
                                block.parent_hash,
                                transactions_to_json(block.transactions))


def write_to_json_file(block_generator):
    with open('btc.blockchaindata', "w+") as f:
        for block in block_generator:
            f.write(block_to_json(block))
