from nltk import Tree
from copy import deepcopy
import codecs
import sys

# http://www.nltk.org/_modules/nltk/treetransforms.html#chomsky_normal_form

HORZMARKOV = 0
VERTMARKOV = 0

LANGUAGE = "eng"

dic_head_rules = {}
default_left_rule = ["leftexcept", "''", "``", "-LRB-", "-RRB-", ".", ":", ","]

default_right_rule = ["rightexcept", "''", "``", "-LRB-", "-RRB-", ".", ":", ","]
default_rule = default_right_rule

def read_from_ptb(filename):
    ptb_file = open(filename, 'r')
    for l in ptb_file:
        tree = Tree.fromstring(l, remove_empty_top_bracketing=False)
        # for st in tree.subtrees():
        #     print "*" + st.pprint()
        original = deepcopy(tree)
        #print tree.pprint()
        chomsky_normal_form(tree, horzMarkov=HORZMARKOV, vertMarkov=VERTMARKOV, childChar='|', parentChar='#')
        #un_chomsky_normal_form(tree)
        #print tree.pprint()
        #print original.pprint() == tree.pprint()
    ptb_file.close()

# Check the head rules to match in each case
# (Now a re-implementation of (Collins, 1999) thesis)
# Lingpeng Kong, lingpenk@cs.cmu.edu

PUNCTSET = set([".", ",", ":", "``", "''"])

def remove_labelChar(n, labelChar = '^'):
    # print n
    # print type(n).__name__
    r_labelChar = labelChar
    if isinstance(n, str) or isinstance(n, unicode):
        if isinstance(n, unicode):
            r_labelChar = labelChar.encode('utf-8')
        return n[:(n.rfind(labelChar) if n.rfind(labelChar) > 0 else len(n))]
    else:
        return [remove_labelChar(x) for x in n]

def findHead(parent,child_list, labelChar='^'):
    if LANGUAGE == "eng":
        return findHeadEnglish(parent,child_list, labelChar)
    else:
        r_parent = str(parent)
        r_child_list = [str(c) for c in child_list]
        x_parent = remove_labelChar(r_parent, labelChar)
        x_child_list = remove_labelChar(r_child_list, labelChar)
        return findHeadChinses(x_parent,x_child_list, labelChar)

def findHeadChinses(parent,child_list, labelChar='^'):
    # print parent, child_list
    if len(child_list) == 1:
        #Unary Rule -> the head must be the only one
        return 0
    # Chinese HeadRule : http://stp.lingfil.uu.se/~nivre/research/chn_headrules.txt
    # @inproceedings{ding2005machine,
    #     title={Machine translation using probabilistic synchronous dependency insertion grammars},
    #     author={Ding, Yuan and Palmer, Martha},
    #     booktitle={Proceedings of the 43rd Annual Meeting on Association for Computational Linguistics},
    #     pages={541--548},
    #     year={2005},
    #     organization={Association for Computational Linguistics}
    # }
    # ADJP    r ADJP JJ;r AD NN CS;r
    if parent == "ADJP":
        s = set(["ADJP", "JJ"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        s = set(["AD", "NN", "CS"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # ADVP    r ADVP AD;r
    if parent == "ADVP":
        s = set(["ADVP", "AD"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # CLP r CLP M;r
    if parent == "CLP":
        s = set(["CLP", "M"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # CP  r DEC SP;l ADVP CS;r CP IP;r
    if parent == "CP":
        s = set(["DEC", "SP"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        s = set(["ADVP", "CS"])
        for i in xrange(len(child_list)):
            if child_list[i] in s:
                return i
        s = set(["CP", "IP"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # DNP r DNP DEG;r DEC;r
    if parent == "DNP":
        s = set(["DNP", "DEG"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        s = set(["DEC"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # DP  l DP DT;l
    if parent == "DP":
        s = set(["DP", "DT"])
        for i in xrange(len(child_list)):
            if child_list[i] in s:
                return i
        return firstNoPunctChild(child_list)
    # DVP r DVP DEV;r
    if parent == "DVP":
        s = set(["DVP", "DEV"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # FRAG    r VV NR NN;r
    if parent == "FRAG":
        s = set(["VV", "NR", "NN"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # INTJ    r INTJ IJ;r
    if parent == "INTJ":
        s = set(["INTJ", "IJ"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # IP  r IP VP;r VV;r
    if parent == "IP":
        s = set(["IP", "VP"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        s = set(["VV"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # LCP r LCP LC;r
    if parent == "LCP":
        s = set(["LCP", "LC"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # LST l LST CD OD;l
    if parent == "LST":
        s = set(["LST", "CD", "OD"])
        for i in xrange(len(child_list)):
            if child_list[i] in s:
                return i
        return firstNoPunctChild(child_list)
    # NP  r NP NN NT NR QP;r
    if parent == "NP":
        s = set(["NP", "NN", "NT", "NR", "QP"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # PP  l PP P;l
    if parent == "PP":
        s = set(["PP", "P"])
        for i in xrange(len(child_list)):
            if child_list[i] in s:
                return i
        return firstNoPunctChild(child_list)
    # PRN r NP IP VP NT NR NN;r
    if parent == "PRN":
        s = set(["NP", "IP", "VP", "NT", "NR", "NN"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # QP  r QP CLP CD OD;r
    if parent == "QP":
        s = set(["QP", "CLP", "CD", "OD"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # UCP r
    if parent == "UCP":
        return lastNoPunctChild(child_list)
    # VCD r VCD VV VA VC VE;r
    if parent == "VCD":
        s = set(["VCD", "VV", "VA", "VC", "VE"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # VCP r VCP VV VA VC VE;r
    if parent == "VCP":
        s = set(["VCP", "VV", "VA", "VC", "VE"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # VNV r VNV VV VA VC VE;r
    if parent == "VNV":
        s = set(["VNV", "VV", "VA", "VC", "VE"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # VP  l VP VA VC VE VV BA LB VCD VSB VRD VNV VCP;l
    if parent == "VP":
        s = set(["VP", "VA", "VC", "VE", "VV", "BA", "LB", "VCD", "VSB", "VRD", "VNV", "VCP"])
        for i in xrange(len(child_list)):
            if child_list[i] in s:
                return i
        return firstNoPunctChild(child_list)
    # VPT r VNV VV VA VC VE;r
    if parent == "VPT":
        s = set(["VNV", "VV", "VA", "VC", "VE"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # VRD r VRD VV VA VC VE;r
    if parent == "VRD":
        s = set(["VRD", "VV", "VA", "VC", "VE"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # VSB r VSB VV VA VC VE;r
    if parent == "VSB":
        s = set(["VSB", "VV", "VA", "VC", "VE"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # WHNP    r WHNP NP NN NT NR QP;r
    if parent == "WHNP":
        s = set(["WHNP", "NP", "NN", "NT", "NR", "QP"])
        for i in reversed(xrange(len(child_list))):
            if child_list[i] in s:
                return i
        return lastNoPunctChild(child_list)
    # WHPP    l WHPP PP P;l
    if parent == "WHPP":
        s = set(["WHPP", "PP", "P"])
        for i in xrange(len(child_list)):
            if child_list[i] in s:
                return i
        return firstNoPunctChild(child_list)
    return firstNoPunctChild(child_list)

# Parent is a string, child_list is a list of string
def findHeadEnglish(parent,child_list, labelChar='^'):
    r_parent = remove_labelChar(parent, labelChar)
    r_child_list = remove_labelChar(child_list, labelChar)
    #print r_child_list
    if len(r_child_list) == 1:
        #Unary Rule -> the head must be the only one
        return 0
    normalHead = findHeaderNormal(r_parent, r_child_list)
    return findHeaderCoord(r_parent, r_child_list, normalHead);

def firstNoPunctChild(child_list):
    for i in xrange(len(child_list)):
        if child_list[i] not in PUNCTSET:
            return i
    return 0

def lastNoPunctChild(child_list):
    for i in reversed(xrange(len(child_list))):
        if child_list[i] not in PUNCTSET:
            return i
    return -1

def test():
    print findHead("NP",["DT", "NNP", "CC", "NNP"])
    print findHead("VP",["ADVP", "VBN", "PP", "NP"])
    print findHead("NP",["NP", "NP",",","NP",",","NP","CC","NP"])




def init_rules_mod_collins():
    # This version from Collins' diss (1999: 236-238)
    # NNS, NN is actually sensible (money, etc.)!
    # QP early isn't; should prefer JJR NN RB
    # remove ADVP; it just shouldn't be there.
    # if two JJ, should take right one (e.g. South Korean)
    # dic_head_rules["ADJP"] = [["left", "NNS", "NN", "$", "QP"], ["right", "JJ"], ["left", "VBN", "VBG", "ADJP", "JJP", "JJR", "NP", "JJS", "DT", "FW", "RBR", "RBS", "SBAR", "RB"]]
    dic_head_rules["ADJP"] = [["left", "$"], ["rightdis", "NNS", "NN", "JJ", "QP", "VBN", "VBG"], ["left", "ADJP"], ["rightdis", "JJP", "JJR", "JJS", "DT", "RB", "RBR", "CD", "IN", "VBD"], ["left", "ADVP", "NP"]]
    dic_head_rules["JJP"] = [["left", "NNS", "NN", "$", "QP", "JJ", "VBN", "VBG", "ADJP", "JJP", "JJR", "NP", "JJS", "DT", "FW", "RBR", "RBS", "SBAR", "RB"]]  # JJP is introduced for NML-like adjective phrases in Vadas' treebank; Chris wishes he hadn't used JJP which should be a POS-tag.
    # ADVP rule rewritten by Chris in Nov 2010 to be rightdis.  This is right! JJ.* is often head and rightmost.
    dic_head_rules["ADVP"] = [["left", "ADVP", "IN"],
                                               ["rightdis", "RB", "RBR", "RBS", "JJ", "JJR", "JJS"],
                                               ["rightdis", "RP", "DT", "NN", "CD", "NP", "VBN", "NNP", "CC", "FW", "NNS", "ADJP", "NML"]]
    dic_head_rules["CONJP"] = [["right", "CC", "RB", "IN"]]
    dic_head_rules["FRAG"] = [["right"]] # crap
    dic_head_rules["INTJ"] = [["left"]]
    dic_head_rules["LST"] = [["right", "LS", ":"]]

    # NML is head in: (NAC-LOC (NML San Antonio) (, ,) (NNP Texas))
    # TODO: NNP should be head (rare cases, could be ignored):
    #   (NAC (NML New York) (NNP Court) (PP of Appeals))
    #   (NAC (NML Prudential Insurance) (NNP Co.) (PP Of America))
    # Chris: This could maybe still do with more thought, but NAC is rare.
    dic_head_rules["NAC"] = [["left", "NN", "NNS", "NML", "NNP", "NNPS", "NP", "NAC", "EX", "$", "CD", "QP", "PRP", "VBG", "JJ", "JJS", "JJR", "ADJP", "JJP", "FW"]]
    dic_head_rules["NX"] = [["right", "NP", "NX"]]

    # Added JJ to PP head table, since it is a head in several cases, e.g.:
    # (PP (JJ next) (PP to them))
    # When you have both JJ and IN daughters, it is invariably "such as" -- not so clear which should be head, but leave as IN
    # should prefer JJ? (PP (JJ such) (IN as) (NP (NN crocidolite)))  Michel thinks we should make JJ a head of PP
    # added SYM as used in new treebanks for symbols filling role of IN
    # Changed PP search to left -- just what you want for conjunction (and consistent with SemanticHeadFinder)
    dic_head_rules["PP"] = [["right", "IN", "TO", "VBG", "VBN", "RP", "FW", "JJ", "SYM"], ["left", "PP"]]

    dic_head_rules["PRN"] = [["left", "VP", "NP", "PP", "SQ", "S", "SINV", "SBAR", "ADJP", "JJP", "ADVP", "INTJ", "WHNP", "NAC", "VBP", "JJ", "NN", "NNP"]]
    dic_head_rules["PRT"] = [["right", "RP"]]
    # add '#' for pounds!!
    dic_head_rules["QP"] = [["left", "$", "IN", "NNS", "NN", "JJ", "CD", "PDT", "DT", "RB", "NCD", "QP", "JJR", "JJS"]]
    # reduced relative clause can be any predicate VP, ADJP, NP, PP.
    # For choosing between NP and PP, really need to know which one is temporal and to choose the other.
    # It's not clear ADVP needs to be in the list at all (delete?).
    dic_head_rules["RRC"] = [["left", "RRC"], ["right", "VP", "ADJP", "JJP", "NP", "PP", "ADVP"]]

    # delete IN -- go for main part of sentence; add FRAG

    dic_head_rules["S"] = [["left", "TO", "VP", "S", "FRAG", "SBAR", "ADJP", "JJP", "UCP", "NP"]]
    dic_head_rules["SBAR"] = [["left", "WHNP", "WHPP", "WHADVP", "WHADJP", "IN", "DT", "S", "SQ", "SINV", "SBAR", "FRAG"]]
    dic_head_rules["SBARQ"] = [["left", "SQ", "S", "SINV", "SBARQ", "FRAG", "SBAR"]]
    # cdm: if you have 2 VP under an SINV, you should really take the 2nd as syntactic head, because the first is a topicalized VP complement of the second, but for now I didn't change this, since it didn't help parsing.  (If it were changed, it'd need to be also changed to the opposite in SemanticHeadFinder.)
    dic_head_rules["SINV"] = [["left", "VBZ", "VBD", "VBP", "VB", "MD", "VBN", "VP", "S", "SINV", "ADJP", "JJP", "NP"]]
    dic_head_rules["SQ"] = [["left", "VBZ", "VBD", "VBP", "VB", "MD", "AUX", "AUXG", "VP", "SQ"]]  # TODO: Should maybe put S before SQ for tag questions. Check.
    dic_head_rules["UCP"] = [["right"]]
    # below is weird!! Make 2 lists, one for good and one for bad heads??
    # VP: added AUX and AUXG to work with Charniak tags
    dic_head_rules["VP"] = [["left", "TO", "VBD", "VBN", "MD", "VBZ", "VB", "VBG", "VBP", "VP", "AUX", "AUXG", "ADJP", "JJP", "NN", "NNS", "JJ", "NP", "NNP"]]
    dic_head_rules["WHADJP"] = [["left", "WRB", "WHADVP", "RB", "JJ", "ADJP", "JJP", "JJR"]]
    dic_head_rules["WHADVP"] = [["right", "WRB", "WHADVP"]]
    dic_head_rules["WHNP"] = [["left", "WDT", "WP", "WP$", "WHADJP", "WHPP", "WHNP"]]
    dic_head_rules["WHPP"] = [["right", "IN", "TO", "FW"]]
    dic_head_rules["X"] = [["right", "S", "VP", "ADJP", "JJP", "NP", "SBAR", "PP", "X"]]
    dic_head_rules["NP"] = [["rightdis", "NN", "NNP", "NNPS", "NNS", "NML", "NX", "POS", "JJR"], ["left", "NP", "PRP"], ["rightdis", "$", "ADJP", "JJP", "PRN", "FW"], ["right", "CD"], ["rightdis", "JJ", "JJS", "RB", "QP", "DT", "WDT", "RBR", "ADVP"]]
    # TODO: seems JJ should be head of NML in this case:
    # (NP (NML (JJ former) (NML Red Sox) (JJ great)) (NNP Luis) (NNP Tiant)),
    dic_head_rules["NML"] = [["rightdis", "NN", "NNP", "NNPS", "NNS", "NX", "NML", "POS", "JJR"], ["left", "NP", "PRP"], ["rightdis", "$", "ADJP", "JJP", "PRN"], ["right", "CD"], ["rightdis", "JJ", "JJS", "RB", "QP", "DT", "WDT", "RBR", "ADVP"]]

    dic_head_rules["POSSP"] = [["right", "POS"]]

    # HJT: Adding the following to deal with oddly formed data in (for example) the Brown corpus
    dic_head_rules["ROOT"] = [["left", "S", "SQ", "SINV", "SBAR", "FRAG"]]
    dic_head_rules["TYPO"] = [["left", "NN", "NP", "NML", "NNP", "NNPS", "TO",
      "VBD", "VBN", "MD", "VBZ", "VB", "VBG", "VBP", "VP", "ADJP", "JJP", "FRAG"]] # for Brown (Roger)
    dic_head_rules["ADV"] = [["right", "RB", "RBR", "RBS", "FW",
      "ADVP", "TO", "CD", "JJR", "JJ", "IN", "NP", "NML", "JJS", "NN"]]

    # SWBD
    dic_head_rules["EDITED"] =  [["left"]]  # crap rule for Switchboard (if don't delete EDITED nodes)
    # in sw2756, a "VB". (copy "VP" to handle this problem, though should really fix it on reading)
    dic_head_rules["VB"] = [["left", "TO", "VBD", "VBN", "MD", "VBZ", "VB", "VBG", "VBP", "VP", "AUX", "AUXG", "ADJP", "JJP", "NN", "NNS", "JJ", "NP", "NNP"]]

    dic_head_rules["META"] =  [["left"]]  # rule for OntoNotes, but maybe should just be deleted in TreeReader??
    dic_head_rules["XS"] =  [["right", "IN"]] # rule for new structure in QP, introduced by Stanford in QPTreeTransformer
    # nonTerminalInfo.put(null] =  [["left"]]  # rule for OntoNotes from Michel, but it would be better to fix this in TreeReader or to use a default rule?

    # todo: Uncomment this line if we always want to take the leftmost if no head rule is defined for the mother category.
    # defaultRule = defaultLeftRule; # Don't exception, take leftmost if no rule defined for a certain parent category

def init_rules_stanford():
    init_rules_mod_collins()
    #  NP: don't want a POS to be the head
    dic_head_rules["NP"] = [["rightdis", "NN", "NNP", "NNPS", "NNS", "NX", "NML", "JJR", "WP" ], ["left", "NP", "PRP"], ["rightdis", "$", "ADJP", "FW"], ["right", "CD"], ["rightdis", "JJ", "JJS", "QP", "DT", "WDT", "NML", "PRN", "RB", "RBR", "ADVP"], ["left", "POS"]]
    # WHNP clauses should have the same sort of head as an NP
    # but it a WHNP has a NP and a WHNP under it, the WHNP should be the head.  E.g.,  (WHNP (WHNP (WP$ whose) (JJ chief) (JJ executive) (NN officer))(, ,) (NP (NNP James) (NNP Gatward))(, ,))
    dic_head_rules["WHNP"] = [["rightdis", "NN", "NNP", "NNPS", "NNS", "NX", "NML", "JJR", "WP"], ["left", "WHNP", "NP"], ["rightdis", "$", "ADJP", "PRN", "FW"], ["right", "CD"], ["rightdis", "JJ", "JJS", "RB", "QP"], ["left", "WHPP", "WHADJP", "WP$", "WDT"]]
    #WHADJP
    dic_head_rules["WHADJP"] = [["left", "ADJP", "JJ", "JJR", "WP"], ["right", "RB"], ["right"]]
    #WHADJP
    dic_head_rules["WHADVP"] = [["rightdis", "WRB", "WHADVP", "RB", "JJ"]] # if not WRB or WHADVP, probably has flat NP structure, allow JJ for "how long" constructions
    # QP: we don't want the first CD to be the semantic head (e.g., "three billion": head should be "billion"), so we go from right to left
    dic_head_rules["QP"] = [["right", "$", "NNS", "NN", "CD", "JJ", "PDT", "DT", "IN", "RB", "NCD", "QP", "JJR", "JJS"]]

    # S, SBAR and SQ clauses should prefer the main verb as the head
    # S: "He considered him a friend" -> we want a friend to be the head
    dic_head_rules["S"] = [["left", "VP", "S", "FRAG", "SBAR", "ADJP", "UCP", "TO"], ["right", "NP"]]

    dic_head_rules["SBAR"] = [["left", "S", "SQ", "SINV", "SBAR", "FRAG", "VP", "WHNP", "WHPP", "WHADVP", "WHADJP", "IN", "DT"]]
    # VP shouldn't be needed in SBAR, but occurs in one buggy tree in PTB3 wsj_1457 and otherwise does no harm

    dic_head_rules["SQ"] = [["left", "VP", "SQ", "ADJP", "VB", "VBZ", "VBD", "VBP", "MD", "AUX", "AUXG"]]


    # UCP take the first element as head
    dic_head_rules["UCP"] = [["left"]]

    # CONJP: we want different heads for "but also" and "but not" and we don't want "not" to be the head in "not to mention"; now make "mention" head of "not to mention"
    dic_head_rules["CONJP"] = [["right", "VB", "JJ", "RB", "IN", "CC"]]

    # FRAG: crap rule needs to be change if you want to parse glosses; but it is correct to have ADJP and ADVP before S because of weird parses of reduced sentences.
    dic_head_rules["FRAG"] = [["left", "IN"], ["right", "RB"], ["left", "NP"], ["left", "ADJP", "ADVP", "FRAG", "S", "SBAR", "VP"]]

    # PRN: sentence first
    dic_head_rules["PRN"] = [["left", "VP", "SQ", "S", "SINV", "SBAR", "NP", "ADJP", "PP", "ADVP", "INTJ", "WHNP", "NAC", "VBP", "JJ", "NN", "NNP"]]

    # add the constituent XS (special node to add a layer in a QP tree introduced in our QPTreeTransformer)
    dic_head_rules["XS"] = [["right", "IN"]]

    # add a rule to deal with the CoNLL data
    dic_head_rules["EMBED"] = [["right", "INTJ"]]


def findHeaderNormal_Stanford(parent, child_list):
    theHead = None

    # We know we have nonterminals underneath
    # (a bit of a Penn Treebank assumption, but).

    rule = dic_head_rules[parent]

    if rule == None:
      if default_rule != None:
        return traverse_locate(child_list, default_rule, True)

    for i in xrange(len(rule)):
      last_resort = (i == (len(rule) - 1))
      theHead = traverse_locate(child_list, rule[i], last_resort)
      if theHead != None:
        break
    return theHead

# def findHeaderNormal_Stanford(parent, child_list):
    # Create Head Rules
    
    # (ROOT
    # (SBARQ
    #   (WHNP (WP Who))
    #   (SQ (VBP am)
    #     (NP (PRP I))
    #     (VP (TO to)
    #       (VP (VB judge))))
    #   (. ?)))
    # WHNP is the head here

    # (ROOT
    #   (SBARQ
    #     (WHNP (WP what))
    #     (SQ
    #       (VP (VBZ is)
    #         (ADJP (JJ wrong))))))
    # ADJP is the head here

    # LPK: Basically, these two rules are global, we will miss. There seems to be no hope here.

    # do VPs with auxiliary as special case
    # looks for copular verbs

    # Another two things we missed here, they conflict with us because we assume (NT -> NT NT) -> head is a deterministic mapping
    

def traverse_locate(child_list, rule, last_resort):
    head_ind = None
    if rule[0] == "left":
        head_ind = findLeftHead(child_list, rule[1:])
    elif rule[0] == "leftdis":
        head_ind = findLeftDisHead(child_list, rule[1:])
    elif rule[0] == "leftexcept":
        head_ind = findLeftExceptHead(child_list, rule[1:])
    elif rule[0] == "right":
        head_ind = findRightHead(child_list, rule[1:])
    elif rule[0] == "rightdis":
        head_ind = findRightDisHead(child_list, rule[1:])
    elif rule[0] == "rightexcept":
        head_ind = findRightExceptHead(child_list, rule[1:])
    else:
        print "error head rule"
        exit()
    

    # what happens if our rule didn't match anything
    if head_ind < 0:
      if last_resort:
          # use the default rule to try to match anything except categoriesToAvoid
          # if that doesn't match, we'll return the left or rightmost child (by
          # setting head_ind).  We want to be careful to ensure that postOperationFix
          # runs exactly once.
          if rule[0].startswith("left"):
              head_ind = 0
              rule = default_left_rule
          else:
              head_ind = len(child_list) - 1
              rule = default_right_rule
        
          child = traverse_locate(child_list, rule, False)

          if child > 0:
              return child
          else:
              return head_ind
        
      else:
        # if we're not the last resort, we can return None to let the next rule try to match
        return None

    return head_ind

# Functions based on Chris Manning's code from Stanford NLP
def findLeftHead(child_list, rule):
    for i in xrange(len(rule)):
        for j in xrange(len(child_list)):
            if rule[i] == child_list[j]:
                return j
    return -1


def findLeftDisHead(child_list, rule):
    for j in xrange(len(child_list)):
        for i in xrange(len(rule)):
            if rule[i] == child_list[j]:
                return j
    return -1

def findLeftExceptHead(child_list, rule):
    for j in xrange(len(child_list)):
        found = True
        for i in xrange(len(rule)):
            if rule[i] == child_list[j]:
                found = False
        if found:
            return j 
    return -1

def findRightHead(child_list, rule):
    for i in xrange(len(rule)):
        for j in reversed(xrange(len(child_list))):
            if rule[i] == child_list[j]:
                return j
    return -1

def findRightDisHead(child_list, rule):
    for j in reversed(xrange(len(child_list))):
        for i in xrange(len(rule)):
            if rule[i] == child_list[j]:
                return j
    return -1

def findRightExceptHead(child_list, rule):
    for j in reversed(xrange(len(child_list))):
        found = True
        for i in xrange(len(rule)):
            if rule[i] == child_list[j]:
                found = False
        if found:
            return j 
    return -1

def findHeaderNormal(parent, child_list):
    # return findHeaderNormal_Stanford(parent, child_list)
    return findHeaderNormal_Collins(parent, child_list)

def findHeaderNormal_Collins(parent, child_list):
        # Rules for NPs
        if parent == "NP":
            # If the last word is tagged POS return (last word)
            if child_list[lastNoPunctChild(child_list)] == "POS":
                return lastNoPunctChild(child_list)

            # Else search from right to left for the first child which is an
            # NN, NNP, NNPS, NNS, NX, POS or JJR
            s = set(["NN", "NNP", "NNPS", "NNS", "NX", "POS", "JJR"])

            for i in reversed(xrange(len(child_list))):
                if child_list[i] in s:
                    return i

            # Else search from left to right for the first child which is an NP
            for i in xrange(len(child_list)):
                if child_list[i] == "NP":
                    return i

            # Else search from right to left for the first child which is a $,
            # ADJP or PRN.
            s = set(["$", "ADJP", "PRN"])
            for i in reversed(xrange(len(child_list))):
                if child_list[i] in s:
                    return i

            # Else search from right to left for the first child which is a CD.
            for i in reversed(xrange(len(child_list))):
                if child_list[i] == ("CD"):
                    return i

            # Else search from right to left for the first child which is a JJ,
            # JJS, RB or QP.
            s = set(["JJ", "JJS", "RB", "QP"])
            for i in reversed(xrange(len(child_list))):
                if child_list[i] in s:
                    return i

            # Else return the last word.
            return lastNoPunctChild(child_list)


        # ADJP -- Left -- NNS QP NN $ ADVP JJ VBN VBG ADJP JJR NP JJS DT FW RBR
        # RBS SBAR RB
        if parent == "ADJP":
            plist = ["NNS", "QP", "NN", "$", "ADVP", "JJ", "VBN", "VBG", "ADJP", "JJR", "NP", "JJS", "DT", "FW", "RBR", "RBS", "SBAR", "RB"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i

            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # ADVP -- Right -- RB RBR RBS FW ADVP TO CD JJR JJ IN NP JJS NN
        if parent == "ADVP":
            plist = ["RB", "RBR", "RBS", "FW", "ADVP", "TO", "CD", "JJR", "JJ", "IN", "NP", "JJS", "NN"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return rightmost
            return lastNoPunctChild(child_list)

        # CONJP -- Right -- CC RB IN
        if parent == "CONJP":
            plist = ["CC", "RB", "IN"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return rightmost
            return lastNoPunctChild(child_list)

        # FRAG -- Right
        if parent == "FRAG":
            return lastNoPunctChild(child_list)

        # INTJ -- Left
        if parent == "INTJ":
            return firstNoPunctChild(child_list)

        # LST -- Right -- LS :
        if parent == "LST":
            plist = ["LS", ":"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return rightmost
            return lastNoPunctChild(child_list)

        # NAC -- Left -- NN NNS NNP NNPS NP NAC EX $ CD QP PRP VBG JJ JJS JJR
        # ADJP FW
        if parent == "NAC":
            plist = ["NN", "NNS", "NNP", "NNPS", "NP", "NAC", "EX", "$", "CD", "QP", "PRP", "VBG", "JJ", "JJS", "JJR", "ADJP", "FW"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # PP -- Right -- IN TO VBG VBN RP FW
        if parent == "PP":
            plist = ["IN", "TO", "VBG", "VBN", "RP", "FW"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i

            # Nothing found, return rightmost
            return lastNoPunctChild(child_list);

        # PRN -- Left
        if parent == "PRN":
            return firstNoPunctChild(child_list)

        # PRT -- Right -- RP
        if parent == "PRT":
            plist = ["RP"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i

            # Nothing found, return rightmost
            return lastNoPunctChild(child_list);

        # QP -- Left -- $ IN NNS NN JJ RB DT CD NCD QP JJR JJS
        if parent == "QP":
            plist = ["$", "IN", "NNS", "NN", "JJ", "RB", "DT", "CD", "NCD", "QP", "JJR", "JJS"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i

            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # RRC -- Right -- VP NP ADVP ADJP PP
        if parent == "RRC":
            plist = ["VP","NP","ADVP","ADJP","PP"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i

            # Nothing found, return rightmost
            return lastNoPunctChild(child_list)

        # S -- Left -- TO IN VP S SBAR ADJP UCP NP
        if parent == "S":
            plist = ["TO", "IN", "VP", "S", "SBAR", "ADJP", "UCP", "NP"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # SBAR -- Left -- WHNP WHPP WHADVP WHADJP IN DT S SQ SINV SBAR FRAG
        if parent == "SBAR":
            plist = ["WHNP", "WHPP", "WHADVP", "WHADJP", "IN", "DT", "S", "SQ", "SINV", "SBAR", "FRAG"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # SBARQ -- Left -- SQ S SINV SBARQ FRAG
        if parent == "SBARQ":
            plist = ["SQ", "S", "SINV", "SBARQ", "FRAG"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # SINV -- Left -- VBZ VBD VBP VB MD VP S SINV ADJP NP
        if parent == "SINV":
            plist = ["VBZ", "VBD", "VBP", "VB", "MD", "VP", "S", "SINV", "ADJP", "NP"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # SQ -- Left -- VBZ VBD VBP VB MD VP SQ
        if parent == "SQ":
            plist = ["VBZ", "VBD", "VBP", "VB", "MD", "VP", "SQ"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # UCP -- Right
        if parent == "UCP":
            return lastNoPunctChild(child_list)

        # VP -- Left -- TO VBD VBN MD VBZ VB VBG VBP VP ADJP NN NNS NP
        if parent == "VP":
            plist = ["TO", "VBD", "VBN", "MD", "VBZ", "VB", "VBG", "VBP", "VP", "ADJP", "NN", "NNS", "NP"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # WHADJP -- Left -- CC WRB JJ ADJP
        if parent == "WHADJP":
            plist = ["CC", "WRB", "JJ", "ADJP"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # WHADVP -- Right -- CC WRB
        if parent == "WHADVP":
            plist = ["CC", "WRB"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return rightmost
            return lastNoPunctChild(child_list);

        # WHNP -- Left -- WDT WP WP$ WHADJP WHPP WHNP
        if parent == "WHNP":
            plist = ["WDT", "WP", "WP$", "WHADJP", "WHPP", "WHNP"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # WHPP -- Right -- IN TO FW
        if parent == "WHPP":
            plist = ["IN", "TO", "FW"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return rightmost
            return lastNoPunctChild(child_list)

        # No rule found, return leftmost
        return firstNoPunctChild(child_list)

def findHeaderCoord(parent, child_list, normalHead):
        # Rules for Coordinated Phrases
        h = normalHead
        if h < len(child_list) - 2:
            if child_list[h+1] == "CC":
                # Y_h Y_h+1 Y_h+2 forms a triple of non-terminals in a coordinating relationship,
                # But since Y_h already the head, nothing happened in unlabeled parsing.
                return normalHead

        if h > 1:
            if child_list[h-1] == "CC":
                # Y_h-2 Y_h-1 Y_h forms a triple of non-terminals in a coordinating relationship,
                # the head is modified to be Y_h-2 in this case

                # Make sure punctuation not selected as the head
                if child_list[h-2] not in PUNCTSET:
                    return h-2

        return normalHead

def lexLabel(tree, labelChar="^"):
    # a little hacky way to label the leaves using the index
    preterminals = [t for t in tree.subtrees(lambda t: t.height() == 2)]
    for i in xrange(len(preterminals)):
        preterminals[i][0] = preterminals[i][0] + labelChar + str(i+1)
    #     leaves[i] = leaves[i] + "-" + str(i+1)
    # print leaves

    for pos in tree.treepositions(order='postorder'):
        st = tree[pos]
        # print st
        # print type(st).__name__
        if isinstance(st, str) or isinstance(st, unicode):
            continue
        else:
            # print "*" + str(st)
            if len(st) == 1:
                # print st.label() + labelChar + findIndex(st[0])
                st.set_label(st.label() + labelChar + findIndex(st[0]))
            else:
                child_list_str = [n.label() if isinstance(n, Tree) else n for n in st]
                child_list = [n for n in st]
                index = findIndex(child_list[findHead(st.label(), child_list_str)])
                st.set_label(st.label() + labelChar + findIndex(index))

    # for t in tree.subtrees():
    #     if isinstance(t, str) or isinstance(n, unicode):
    #         continue
    #     else:
    #         t.set_label(t.label() + labelChar + getLexLabelNT(t))
    #     #print "*" + str(tree)

# return a string representing a int as index
# def getLexLabelNT(node, labelChar='^'):
#     # print "we are at " + str(node)
#     if isinstance(node, str) or isinstance(n, unicode):
#         return findIndex(node, labelChar)
#     if isinstance(node,Tree):
#         if node.height() == 2:
#             return findIndex(node[0])
#         else:
#             child_list_str = [n.label() if isinstance(n, Tree) else n for n in node]
#             child_list = [n for n in node]
#             return getLexLabelNT(child_list[findHead(node.label(), child_list_str)], labelChar)

# e.g. for "word^3" return '3'
def findIndex(s, labelChar='^'):
    r_labelChar = labelChar
    if isinstance(s, str) or isinstance(s, unicode):
        if isinstance(s, unicode):
            r_labelChar = r_labelChar.encode('utf-8')
        return s[s.rfind(r_labelChar)+1:]
    else:
        m = s.label()
        return m[m.rfind(labelChar)+1:]

def getParentDic(lexTree, labelChar='^'):
    lt = deepcopy(lexTree)
    # remove the TOP label
    #if lt.label().startswith('TOP'):
        # print lt.label()
    #    lt = lt[0]
    # print lt.pprint()

    # build a parent set
    dep_set = {}
    label_set = {}
    # add root to the tree
    dep_set[findIndex(lt.label())] = '0'
    # label for the root is just ROOT
    label_set[findIndex(lt.label())] = 'ROOT'
    for pos in lt.treepositions(order='preorder'):
        # print pos
        nt = lt[pos]
        #print unicode(nt)
        if isinstance(nt, Tree) and nt.height() != 2 and len(nt) > 1:
            ind_p = findIndex(nt.label())
            phrase_label = remove_labelChar(nt.label())
            #print nt.label()
            #print phrase_label
            head_label = '*'
            for c in nt:
                ind_c = findIndex(c.label() if isinstance(c, Tree) else c)
                if ind_c == ind_p and isinstance(c, Tree) and c.height()!=2:
                    head_label = remove_labelChar(c.label())
            for c in nt:
                ind_c = findIndex(c.label() if isinstance(c, Tree) else c)
                #print ind_c
                if not ind_c == ind_p:
                    dependent_label = '*'
                    if isinstance(c, Tree) and c.height()!=2:
                        dependent_label = remove_labelChar(c.label())
                    dep_set[ind_c] = ind_p
                    label_set[ind_c] = phrase_label + "+" + head_label + "+" + dependent_label
    # print dep_set, label_set
    return dep_set, label_set

def generateDep(ot, labelChar='^'):
    lt = deepcopy(ot)
    lexLabel(lt, labelChar)
    dep_set, dep_label_set = getParentDic(lt, labelChar)

    conll_lines = []
    preterminals = [t for t in lt.subtrees(lambda t: t.height() == 2)]
    for i in xrange(len(preterminals)):
        word = remove_labelChar(preterminals[i][0], labelChar)
        pos = remove_labelChar(preterminals[i].label(), labelChar)
        index = findIndex(preterminals[i][0], labelChar)
        parent = dep_set[index]
        label = dep_label_set[index]
        conll_lines.append([index, word, pos, parent, label])
    return conll_lines

def print_conll_lines(clines, wt):
    # 1 Influential _   JJ  JJ  _   2   _   _
    for l in clines:
        wt.write(l[0] + "\t" + l[1] + "\t_\t" + l[2] + "\t" + l[2] + "\t_\t" + l[3] + "\t"+ l[4] +"\t_\t_\n")

def chomsky_normal_form(tree, horzMarkov=None, vertMarkov=0, childChar="|", parentChar="#"):
    # assume all subtrees have homogeneous children
    # assume all terminals have no siblings

    # A semi-hack to have elegant looking code below.  As a result,
    # any subtree with a branching factor greater than 999 will be incorrectly truncated.
    if horzMarkov is None: horzMarkov = 999

    # Traverse the tree depth-first keeping a list of ancestor nodes to the root.
    # I chose not to use the tree.treepositions() method since it requires
    # two traversals of the tree (one to get the positions, one to iterate
    # over them) and node access time is proportional to the height of the node.
    # This method is 7x faster which helps when parsing 40,000 sentences.

    nodeList = [(tree, [tree.label()])]
    # print nodeList

    while nodeList != []:
        node, parent = nodeList.pop()

        if isinstance(node,Tree):
            # for the node and the parent, we want to know which is the index of the head in Collins head rule
            # which basically is the break point of the binarization
            child_list = [n.label() for n in node if isinstance(n, Tree)]
            if len(child_list) == 0:
                continue

            # print parent
            # print str(node.label()) + "\t-->\t" + "\t".join(child_list)

            # The head postion is determined by the collins rule
            head_postion = findHead(node.label(),child_list)
            head = child_list[head_postion]
            #print child_list[head_postion]

            # parent annotation
            parentString = ""
            originalNode = node.label()
            if vertMarkov != 0 and node != tree and isinstance(node[0],Tree):
                parentString = "%s<%s>" % (parentChar, "-".join(parent))
                node.set_label(node.label() + parentString)
                # Attach the higher level parent to the parent list and then pass the into the agenda later
                # The originalNode here is the direct parent of the child node
                parent = [originalNode] + parent[:vertMarkov - 1]

            # add children to the agenda before we mess with them
            for child in node:
                nodeList.append((child, parent))

            # chomsky normal form factorization
            if len(node) > 2:
                childNodes = [child.label() for child in node]
                nodeCopy = node.copy()
                node[0:] = [] # delete the children

                curNode = node
                numChildren = len(nodeCopy)

                for i in range(1,numChildren - 1):
                    #if factor == "right":
                    next_step_right = ((i+1) < head_postion)
                    if i < head_postion:
                        #every time we going to the right, the left context in are consumed by one
                        del childNodes[0]
                        #newHead = "%s%s<%s>%s" % (originalNode, childChar, "-".join(childNodes[i:min([i+horzMarkov,numChildren])]),parentString) # create new head
                        #if next_step_right:
                        #    newHead = "%s%s<%s>%s" % (originalNode, childChar, ("" if len(childNodes)<=2 else "l-") + "-".join(childNodes[:min(len(childNodes),horzMarkov)]),parentString) # create new head
                        #else:
                        #    newHead = "%s%s<%s>%s" % (originalNode, childChar, ("" if len(childNodes)<=2 else "r-") + "-".join(childNodes[max(0,len(childNodes)-horzMarkov):]),parentString) # create new head
                        #if LANGUAGE == "chn":
                        #    newHead = "%s%s-%s" % (originalNode, childChar, head)
                        #else:
                        newHead = "%s%s" % (originalNode, childChar)
                        newNode = Tree(newHead, [])
                        curNode[0:] = [nodeCopy.pop(0), newNode]

                    else:
                        del childNodes[-1]
                        #newHead = "%s%s<%s>%s" % (originalNode, childChar, "-".join(childNodes[max([numChildren-i-horzMarkov,0]):-i]),parentString)
                        #if next_step_right:
                        #    newHead = "%s%s<%s>%s" % (originalNode, childChar, ("" if len(childNodes)<=2 else "l-") + "-".join(childNodes[:min(len(childNodes),horzMarkov)]),parentString) # create new head
                        #else:
                        #    newHead = "%s%s<%s>%s" % (originalNode, childChar, ("" if len(childNodes)<=2 else "r-") + "-".join(childNodes[max(0,len(childNodes)-horzMarkov):]),parentString) # create new head
                        #if LANGUAGE == "chn":
                        #    newHead = "%s%s-%s" % (originalNode, childChar, head)
                        #else:
                        newHead = "%s%s" % (originalNode, childChar)
                        newNode = Tree(newHead, [])
                        curNode[0:] = [newNode, nodeCopy.pop()]


                    curNode = newNode

                curNode[0:] = [child for child in nodeCopy]


def un_chomsky_normal_form(tree, expandUnary = True, childChar = "|", parentChar = "#", unaryChar = "+"):
    # Traverse the tree-depth first keeping a pointer to the parent for modification purposes.
    nodeList = [(tree,[])]
    while nodeList != []:
        node,parent = nodeList.pop()
        if isinstance(node,Tree):
            # if the node contains the 'childChar' character it means that
            # it is an artificial node and can be removed, although we still need
            # to move its children to its parent
            childIndex = node.label().find(childChar)
            if childIndex != -1:
                nodeIndex = parent.index(node)
                parent.remove(parent[nodeIndex])
                # Generated node was on the left if the nodeIndex is 0 which
                # means the grammar was left factored.  We must insert the children
                # at the beginning of the parent's children
                # if nodeIndex == 0:
                parent.insert(nodeIndex,node[0])
                parent.insert(nodeIndex+1,node[1])
                # else:
                #     parent.extend([node[0],node[1]])

                # parent is now the current node so the children of parent will be added to the agenda
                node = parent
            else:
                parentIndex = node.label().find(parentChar)
                if parentIndex != -1:
                    # strip the node name of the parent annotation
                    node.set_label(node.label()[:parentIndex])

                # expand collapsed unary productions
                if expandUnary == True:
                    unaryIndex = node.label().find(unaryChar)
                    if unaryIndex != -1:
                        newNode = Tree(node.label()[unaryIndex + 1:], [i for i in node])
                        node.set_label(node.label()[:unaryIndex])
                        node[0:] = [newNode]

            for child in node:
                nodeList.append((child,node))
        #print "#" + str(tree)


def collapse_unary(tree, collapsePOS = False, collapseRoot = False, joinChar = "+"):
    """
    Collapse subtrees with a single child (ie. unary productions)
    into a new non-terminal (Tree node) joined by 'joinChar'.
    This is useful when working with algorithms that do not allow
    unary productions, and completely removing the unary productions
    would require loss of useful information.  The Tree is modified
    directly (since it is passed by reference) and no value is returned.
    :param tree: The Tree to be collapsed
    :type  tree: Tree
    :param collapsePOS: 'False' (default) will not collapse the parent of leaf nodes (ie.
                        Part-of-Speech tags) since they are always unary productions
    :type  collapsePOS: bool
    :param collapseRoot: 'False' (default) will not modify the root production
                         if it is unary.  For the Penn WSJ treebank corpus, this corresponds
                         to the TOP -> productions.
    :type collapseRoot: bool
    :param joinChar: A string used to connect collapsed node values (default = "+")
    :type  joinChar: str
    """

    if collapseRoot == False and isinstance(tree, Tree) and len(tree) == 1:
        nodeList = [tree[0]]
    else:
        nodeList = [tree]

    # depth-first traversal of tree
    while nodeList != []:
        node = nodeList.pop()
        if isinstance(node,Tree):
            if len(node) == 1 and isinstance(node[0], Tree) and (collapsePOS == True or isinstance(node[0,0], Tree)):
                node.set_label(node.label() + joinChar + node[0].label())
                node[0:] = [child for child in node[0]]
                # since we assigned the child's children to the current node,
                # evaluate the current node again
                nodeList.append(node)
            else:
                for child in node:
                    nodeList.append(child)

def flat_print(t):
    # print the tree in one single line
    return t._pprint_flat(nodesep='', parens='()', quotes=False)

# this actually assumes that every bin rule contains one dep, like we assumed in the paper.
def get_binarize_lex(otree, labelChar='^'):
    work_tree = deepcopy(otree)
    lexLabel(work_tree)
    #print work_tree
    parent_dic, dep_label_set = getParentDic(work_tree)
    # for x in parent_dic:
    #     print x, parent_dic[x]
    work_tree = deepcopy(otree)
    chomsky_normal_form(work_tree, horzMarkov=HORZMARKOV, vertMarkov=VERTMARKOV)


    preterminals = [t for t in work_tree.subtrees(lambda t: t.height() == 2)]
    for i in xrange(len(preterminals)):
        preterminals[i][0] = preterminals[i][0] + labelChar + str(i+1)
    # print work_tree
    # print flat_print(work_tree)
    for pos in work_tree.treepositions(order='postorder'):
        # print pos
        t = work_tree[pos]
        if isinstance(t, str) or isinstance(t, unicode):
            continue
        else:
            if len(t) == 1:
                t.set_label(t.label() + labelChar + findIndex(t[0]))
            else:

                x_ind = findIndex(t[0])
                y_ind = findIndex(t[1])
                #print parent_dic
                if parent_dic[x_ind] == y_ind:
                    t.set_label(t.label() + labelChar + y_ind)
                else:
                    t.set_label(t.label() + labelChar + x_ind)

    # for t in work_tree.subtrees():
    #     if isinstance(t, str) or isinstance(n, unicode):
    #         continue
    #     else:
    #         t.set_label(t.label() + labelChar + find_lex_head_bin(t, parent_dic))
    return work_tree

# def find_lex_head_bin(nt, parent_dic, labelChar='^'):
#     if isinstance(nt, str) or isinstance(n, unicode):
#         return findIndex(nt)
#     if isinstance(nt, Tree):
#         if nt.height() == 2:
#             return findIndex(nt[0])
#         else:
#             if len(nt) == 1:
#                 return find_lex_head_bin(nt[0], parent_dic)
#             else:
#                 if find_lex_head_bin(nt[0], parent_dic) in parent_dic:
#                     if parent_dic[find_lex_head_bin(nt[0], parent_dic)] == find_lex_head_bin(nt[1], parent_dic):
#                         return find_lex_head_bin(nt[1], parent_dic)
#                     else:
#                         return find_lex_head_bin(nt[0], parent_dic)
#                 else:
#                     return find_lex_head_bin(nt[1], parent_dic)

# The function take a lex bin tree and return all the bin (and unary) rules as a set
# the rule is represented as {unary?} {X} {Y} {Z} {DIR}, we will add the {id} later
def generate_rules(lex_bin_tree):
    rule_set = set([])
    for st in lex_bin_tree.subtrees():
        if isinstance(st, str) or isinstance(st, unicode):
            # these are leaves, just skip
            continue
        if isinstance(st, Tree):
            if st.height() == 2:
                # preterminals also don't generate rules
                continue
            else:
                rule = get_rule_for_nt(st)[0]
                rule_set.add(rule)
    return rule_set

def get_rule_for_nt(st):
    if len(st) == 1:
        # unary rule
        X = remove_labelChar(st.label())
        Y = remove_labelChar(st[0].label())
        X_ind = findIndex(st.label())
        rule = "1 " + X + " " + Y
        h = str(int(X_ind) - 1)
        m = str(int(X_ind) - 1)
    else:
        # not unary rule
        X = remove_labelChar(st.label())
        Y = remove_labelChar(st[0].label())
        Z = remove_labelChar(st[1].label())

        X_ind = findIndex(st.label())
        Y_ind = findIndex(st[0].label())
        Z_ind = findIndex(st[1].label())

        d = ('0' if X_ind == Y_ind else '1')
        rule = "0 " + X + " " + Y + " " + Z + " " + d
        h = str(int(X_ind) - 1)
        m = str(int(Z_ind) - 1) if X_ind == Y_ind else str(int(Y_ind) - 1)
    return (rule, h, m)

def get_span_info(nt, rule_to_ind_dict):
    # Given a non-terminal form a indexed tree (^index), generate it's span information
    # Concretely, i, j, k, h, m and rule_num (lookup from the rule_to_ind_dict)
    # Assume it is a tree here and is not a preterminal.

    # First, go all the way down to the left to see the left boundary
    pointer = nt
    while not (isinstance(pointer,str) or isinstance(pointer, unicode)):
        pointer = pointer[0]
    span_left = str(int(findIndex(pointer)) - 1)
    # All the way right to find the right boundary
    pointer = nt
    while not (isinstance(pointer,str) or isinstance(pointer, unicode)):
        if len(pointer) > 1:
            pointer = pointer[1]
        else:
            # unary
            pointer = pointer[0]
    span_right = str(int(findIndex(pointer)) - 1)
    pointer = nt[0]
    while not (isinstance(pointer,str) or isinstance(pointer, unicode)):
        if len(pointer) > 1:
            pointer = pointer[1]
        else:
            # unary
            pointer = pointer[0]
    span_split = str(int(findIndex(pointer)) - 1)

    # we will find the h and m and the rule_num
    rule, h, m = get_rule_for_nt(nt)
    if rule in rule_to_ind_dict:
        return (span_left, span_split, span_right, h, m, rule_to_ind_dict[rule])
    else:
        sys.stderr.write("None rule found!")
        return None




#################################################################
# Demonstration
#################################################################

def dep():
    for l in sys.stdin:
        t = Tree.fromstring(sentence)
        g = generateDep(t)
        for d in g:
            index = int(d[0])
            word = d[1]
            head = int(d[3])
            if head == 0:
                print "label(%s-%d, %s-%d)"%("ROOT", -1, word, index-1)
            else:
                print "label(%s-%d, %s-%d)"%(g[head-1][1], head-1, word, index-1)
        print

def demo():

    """
    A demonstration showing how each tree transform can be used.
    """

    # from nltk.draw.tree import draw_trees
    from nltk import tree, treetransforms
    from copy import deepcopy

    # original tree from WSJ bracketed text
    sentence = """(S
  (S
    (VP
      (VBN Turned)
      (ADVP (RB loose))
      (PP
        (IN in)
        (NP
          (NP (NNP Shane) (NNP Longman) (POS 's))
          (NN trading)
          (NN room)))))
  (, ,)
  (NP (DT the) (NN yuppie) (NNS dealers))
  (VP (AUX do) (NP (NP (RB little)) (ADJP (RB right))))
  (. .))"""

    sentence = """(TOP (S (NP (NP (JJ Influential) (NNS members)) (PP (IN of) (NP (DT the) (NNP House) (NNP Ways) (CC and) (NNP Means) (NNP Committee)))) (VP (VBD introduced) (NP (NP (NN legislation)) (SBAR (WHNP (WDT that)) (S (VP (MD would) (VP (VB restrict) (SBAR (WHADVP (WRB how)) (S (NP (DT the) (JJ new) (NN savings-and-loan) (NN bailout) (NN agency)) (VP (MD can) (VP (VB raise) (NP (NN capital)))))) (, ,) (S (VP (VBG creating) (NP (NP (DT another) (JJ potential) (NN obstacle)) (PP (TO to) (NP (NP (NP (DT the) (NN government) (POS 's)) (NN sale)) (PP (IN of) (NP (JJ sick) (NNS thrifts)))))))))))))) (. .)))"""
    sentence = """(TOP (S (NP (NNP Ms.) (NNP Haag)) (VP (VBZ plays) (NP (NNP Elianti))) (. .)))"""
    #sentence = """(S (PP (IN In) (NP (NP (DT an) (NNP Oct.) (CD 19) (NN review)) (PP (IN of) (NP (`` ``) (NP (DT The) (NN Misanthrope)) ('' '') (PP (IN at) (NP (NP (NNP Chicago) (POS 's)) (NNP Goodman) (NNP Theatre))))) (PRN (-LRB- -LRB-) (`` ``) (S (NP (VBN Revitalized) (NNS Classics)) (VP (VBP Take) (NP (DT the) (NN Stage)) (PP (IN in) (NP (NNP Windy) (NNP City))))) (, ,) ('' '') (NP (NN Leisure) (CC &) (NNS Arts)) (-RRB- -RRB-)))) (, ,) (NP (NP (NP (DT the) (NN role)) (PP (IN of) (NP (NNP Celimene)))) (, ,) (VP (VBN played) (PP (IN by) (NP (NNP Kim) (NNP Cattrall)))) (, ,)) (VP (VBD was) (VP (ADVP (RB mistakenly)) (VBN attributed) (PP (TO to) (NP (NNP Christina) (NNP Haag))))) (. .))"""
    #sentence = """(S (NP (NNP Ms.) (NNP Haag)) (VP (VBZ plays) (NP (NNP Elianti))) (. .))"""
    #sentence = """(PRN (S (VP (VB See))))"""
    # sentence = """(M (S (A (B (B1 b1) (B2 b2) (B3 (BB1 bb1) (BB2 bb2) (BB3 bb3) ) (B4 b4)) (C (C1 c1) (C2 c2)  ) (D d) (E e) (F f) (G g)) (W w)))"""

    #chinese_sample = open('chinese_sample','r')
    #for x in chinese_sample:
    #    sentence = x
    #    break
    #sentence = sentence.decode('utf-8')

    t = tree.Tree.fromstring(sentence, remove_empty_top_bracketing=False)
    print flat_print(t)
    # collapse subtrees with only one child
    lTree = deepcopy(t)
    lexLabel(lTree)
    print flat_print(lTree)

    deps = generateDep(t)
    print_conll_lines(deps,sys.stdout)


    collapsedTree = deepcopy(t)
    #collapse_unary(collapsedTree)

    # convert the tree to CNF
    # cnfTree = deepcopy(collapsedTree)
    # chomsky_normal_form(cnfTree)

    # convert the tree to CNF with parent annotation (one level) and horizontal smoothing of order two
    parentTree = deepcopy(collapsedTree)
    chomsky_normal_form(parentTree, horzMarkov=HORZMARKOV, vertMarkov=VERTMARKOV)

    # convert the tree back to its original form (used to make CYK results comparable)
    original = deepcopy(parentTree)
    un_chomsky_normal_form(original)

    # convert tree back to bracketed text
    sentence2 = original.pprint()
    print(t.pprint())
    print(sentence2)
    #print("Sentences the same? ", str(sentence2) == str(t.pprint()))

    # draw_trees(t, collapsedTree, cnfTree, parentTree, original)
    #print t, collapsedTree, cnfTree, parentTree, original
    print parentTree

    collapse_unary(t)
    t = get_binarize_lex(t)
    print "*" + unicode(t)
    rule_set = generate_rules(t)
    for x in rule_set:
        print x

    chomsky_normal_form(collapsedTree, horzMarkov=HORZMARKOV, vertMarkov=VERTMARKOV)
    print "**" + unicode(collapsedTree)

if __name__ == '__main__':
    demo()
    #dep()
    #test()

    #read_from_ptb('/media/lingpenk/Data/PhraseDep/treebank/dev.1.notraces')
