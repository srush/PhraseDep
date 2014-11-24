import java.io.File;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;

import edu.stanford.nlp.trees.Tree;
import edu.stanford.nlp.trees.TreeReader;
import edu.stanford.nlp.trees.international.pennchinese.CTBErrorCorrectingTreeNormalizer;
import edu.stanford.nlp.trees.international.pennchinese.CTBTreeReaderFactory;

public class Extractor {
	public static ArrayList<Tree> readTrees(String filePath) {
		ArrayList<Tree> trees = new ArrayList<Tree>();
		try {
			CTBTreeReaderFactory ctb_f = new CTBTreeReaderFactory(
					new CTBErrorCorrectingTreeNormalizer());
			TreeReader ptr = ctb_f.newTreeReader(new InputStreamReader(
					new FileInputStream(filePath), "gbk"));

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

	private static int getNumber(String filename) {
		String numberStr = filename.substring(filename.lastIndexOf('_') + 1,
				filename.lastIndexOf('.'));
		return Integer.parseInt(numberStr);
	}

	public static void main(String[] args) {
		File dir = new File("bracketed");
		LineWriter lw_train = new LineWriter("train");
		LineWriter lw_dev = new LineWriter("dev");
		LineWriter lw_test = new LineWriter("test");
		for (File f : dir.listFiles()) {
			if (!f.getName().endsWith("fid"))
				continue;
			// System.err.print(f.getName());
			int index = getNumber(f.getName());
			// System.err.println("\t" + index);
			ArrayList<Tree> treebank = readTrees(f.getAbsolutePath());
			// Articles 001-270 and 440-1151 were used for training,
			if ((index >= 1 && index <= 270) || (index >= 440 && index <= 1151)) {
				for (Tree t : treebank) {
					lw_train.writeln((t.toString()));
				}
			}
			// articles 301-325 were used as development data, and 
			if ((index >= 301 && index <= 325)) {
				for (Tree t : treebank) {
					lw_dev.writeln((t.toString()));
				}
			}
			// articles 271-300 were used for evaluation.
			if ((index >= 271 && index <= 300)) {
				for (Tree t : treebank) {
					lw_test.writeln((t.toString()));
				}
			}
		}
		lw_train.closeAll();
		lw_dev.closeAll();
		lw_test.closeAll();
	}
}
