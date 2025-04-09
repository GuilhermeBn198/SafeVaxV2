
// Para Huffman:
#include <map>
#include <queue>

// ----- IMPLEMENTAÇÃO DO HUFFMAN -----
struct HuffmanNode {
  char ch; int freq;
  HuffmanNode *left, *right;
  HuffmanNode(char _ch, int _freq) : ch(_ch), freq(_freq), left(NULL), right(NULL) {}
};
struct CompareNode {
  bool operator()(HuffmanNode* const & n1, HuffmanNode* const & n2) {
    return n1->freq > n2->freq;
  }
};

HuffmanNode* huffmanTree = NULL;
std::map<char, String> huffmanCodes;

void buildHuffmanCodes(HuffmanNode* root, String code = "") {
  if (!root) return;
  if (!root->left && !root->right) {
    huffmanCodes[root->ch] = code;
  }
  buildHuffmanCodes(root->left,  code + "0");
  buildHuffmanCodes(root->right, code + "1");
}

HuffmanNode* buildHuffmanTree(const String &data) {
  Serial.println("[DEBUG] Construindo árvore Huffman...");
  std::map<char,int> freq;
  for (char c : data) freq[c]++;
  std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, CompareNode> pq;
  for (auto &p : freq) pq.push(new HuffmanNode(p.first, p.second));
  while (pq.size() > 1) {
    HuffmanNode* l = pq.top(); pq.pop();
    HuffmanNode* r = pq.top(); pq.pop();
    HuffmanNode* m = new HuffmanNode('\0', l->freq + r->freq);
    m->left = l; m->right = r;
    pq.push(m);
  }
  Serial.println("[DEBUG] Árvore Huffman construída.");
  return pq.top();
}

String huffmanCompress(const String &data) {
  Serial.println("[DEBUG] Compactando payload com Huffman...");
  String out;
  for (char c : data) out += huffmanCodes[c];
  Serial.println("[DEBUG] Payload compactado.");
  return out;
}

String huffmanDecompress(const String &bits, HuffmanNode* root) {
  Serial.println("[DEBUG] Descompactando payload Huffman...");
  String out;
  HuffmanNode* curr = root;
  for (char b : bits) {
    curr = (b == '0') ? curr->left : curr->right;
    if (!curr->left && !curr->right) {
      out += curr->ch;
      curr = root;
    }
  }
  Serial.println("[DEBUG] Payload descompactado.");
  return out;
}
// ----- FIM DO HUFFMAN -----