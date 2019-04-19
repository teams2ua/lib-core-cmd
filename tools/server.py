from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse
from blockchain.datas import *
from datetime import datetime
import itertools
import json


def block_header_to_json_object(block_header):
    return {"hash": block_header.hash,
            "height": block_header.height,
            "time": datetime.utcfromtimestamp(block_header.unixTimeInSec).strftime("%Y-%m-%dT%H:%M:%SZ")}


def block_to_json_object(block):
    ans = block_header_to_json_object(block.header)
    ans["txs"] = [tr.txHash for tr in block.transactions]
    return ans


def transaction_input_to_json_object(transaction_input):
    return {"input_index": transaction_input.index,
            "output_hash": transaction_input.transaction_output.transaction_hash,
            "output_index": transaction_input.transaction_output.index,
            "value": transaction_input.transaction_output.value,
            "address": transaction_input.transaction_output.address,
            "script_signature": "160014676ad7a7a8c5e3be9c32c1245587a54231e24628"}


def transaction_output_to_json_object(transaction_output):
    return {"output_index": transaction_output.index,
            "value": transaction_output.value,
            "address": transaction_output.address,
            "script_hex": "a9148ca605c9bfa085471d3a03333026ac28878a731287"}


def transaction_to_json_object(transaction):
    return {"hash": transaction.txHash,
            "received_at": datetime.utcfromtimestamp(transaction.unixTimeInSec).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "lock_time": 525285,
            "block": block_header_to_json_object(transaction.block_header),
            "inputs": [transaction_input_to_json_object(inp) for inp in transaction.inputs],
            "outputs": [transaction_output_to_json_object(outp) for outp in transaction.outputs],
            "fees": transaction.get_fee(),
            "amount": transaction.get_amount(),
            "confirmations": 45580}


def transactions_json(trx, truncated):
    return {"truncated": str(truncated).lower(),"txs": [transaction_to_json_object(t) for t in trx]}


class Explorer:
    def __init__(self, blockchain):
        self.blocks = blockchain.blocks
        self.index = {}
        for b, index in zip(self.blocks, itertools.count()):
            self.index[b.header.hash] = index

    def get_current_block(self):
        return self.blocks[-1]

    def get_transactions(self, addresses, block_hash, limit=200):
        if block_hash is None:
            block_hash = self.blocks[0].header.hash
        if block_hash not in self.index:
            return [], False
        res = []
        truncate = False
        for block in self.blocks[self.index[block_hash]:]:
            for tr in block.transactions:
                if not addresses.isdisjoint({inp for inp in tr.inputs}.union({out.address for out in tr.outputs})):
                    if truncate:
                        return res, True
                    res.append(tr)
            # we truncate on the level of blocks, if there will be no more transaction this is considered as
            # not truncated, so we flag the variable and looks what will happened next
            if len(res) >= limit:
                truncate = True
        return res, False


if __name__ == "__main__":
    # if there are no data.pkl, generate one with gen_data.py
    explorer_data = Explorer(deserialize_from_pickle_file("data.pkl"))


    class ExplorerServer(BaseHTTPRequestHandler):
        def do_GET(self):
            try:
                print(self.path)
                parsed = urlparse(self.path)
                splitted = parsed.path.split('/')
                json_object = None
                if len(splitted) < 4:
                    raise ValueError("Request is wrong should be in form blockchain/v2/btc/...")
                if splitted[4] == 'syncToken':
                    json_object = {"token": "7179cff0-3dab-4f15-b035-b3f2b49d2269"}
                if splitted[4] == 'blocks':
                    print("Blocks command!!!")
                    if len(splitted) < 5:
                        raise ValueError("Not enough arguments for block command")
                    if splitted[5] == 'current':
                        json_object = block_to_json_object(explorer_data.get_current_block())
                elif splitted[4] == 'addresses':
                    if len(splitted) < 6:
                        raise ValueError("Not enough arguments for addresses command")
                    if splitted[6] != "transactions":
                        raise ValueError("'transaction' is the only supported operation on addresses")
                    addresses = set(splitted[5].split(','))
                    block_hash = None
                    if "=" in parsed.query:
                        dict(param.split('=') for param in parsed.query.split('&')).get('blockHash', None)
                    json_object = transactions_json(*explorer_data.get_transactions(addresses, block_hash))
                self.send_response(200)
                self.send_header('Content-type', 'application/json; charset=utf-8')
                self.end_headers()
                if json_object:
                    self.wfile.write(json.dumps(json_object).encode('utf-8'))
            except ValueError:
                self.send_error(400, )


    httpd = HTTPServer(('127.0.0.1', 8080), ExplorerServer)
    httpd.serve_forever()
