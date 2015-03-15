import java.io.FileReader;
import java.util.ArrayList;
import java.util.Properties;

import edu.stanford.nlp.parser.lexparser.TreebankLangParserParams;
import edu.stanford.nlp.trees.GrammaticalStructure;
import edu.stanford.nlp.trees.PennTreeReader;
import edu.stanford.nlp.trees.Tree;
import edu.stanford.nlp.trees.TreeGraphNode;
import edu.stanford.nlp.util.Filters;
import edu.stanford.nlp.util.ReflectionLoading;
import edu.stanford.nlp.util.StringUtils;


public class StanfordHeadRuleFinder {
	public static ArrayList<Tree> readTrees(String filepath) {
		ArrayList<Tree> trees = new ArrayList<Tree>();
		try {
			PennTreeReader ptr = new PennTreeReader(new FileReader(filepath));
			Tree t = ptr.readTree();
			while (t != null) {
				trees.add(t);
				t = ptr.readTree();
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return trees;
	}
	
	public static void generateRules(ArrayList<Tree> trees, TreebankLangParserParams params){
		
	}
	
	public static void generateParts(ArrayList<Tree> trees, TreebankLangParserParams params){
		
	}
	
	public static void generateConll(ArrayList<Tree> trees, TreebankLangParserParams params){
		for(Tree t : trees){
			GrammaticalStructure gs = params.getGrammaticalStructure(t, Filters.acceptFilter(), params.typedDependencyHeadFinder());
			//System.out.println(gs.toString());
			GrammaticalStructure.printDependencies(gs, gs.typedDependencies(false), t, true, false);
		}
	}
	
	public static void main(String[] args){
		Properties props = StringUtils.argsToProperties(args);
		
		ArrayList<Tree> trees = readTrees("test.1.notraces");
	    TreebankLangParserParams params = ReflectionLoading.loadByReflection("edu.stanford.nlp.parser.lexparser.EnglishTreebankParserParams");
	    
	    
	    generateConll(trees, params);
	    
	    // Head to determine the Head for a tree node
		for(Tree t : trees){
			
			GrammaticalStructure gs = params.getGrammaticalStructure(t, Filters.acceptFilter(), params.typedDependencyHeadFinder());
			System.out.println(gs.toString());
			TreeGraphNode tgn = gs.root();
			System.out.println("=======0");
			System.out.println(tgn.label());
			
			System.out.println("=======1");
			for(Tree subtree : t.preOrderNodeList()){
				if(!(subtree.isLeaf() || subtree.isPreTerminal())){
					System.out.println("=======2");
					subtree.pennPrint();
					System.out.println("=======3");
					Tree headTree = params.typedDependencyHeadFinder().determineHead(subtree);
					headTree.pennPrint();
					System.out.println("=======4");
				}
			}
			break;
		}
	}
}

