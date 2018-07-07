package com.example.androidex;

import android.app.Activity;
import android.graphics.Color;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;

public class MainActivity2 extends Activity{
	
	LinearLayout linear;
	EditText data;
	Button btn;
	OnClickListener ltn1, ltn2;
  	BackThread mThread;
	
    // Constants for puzzle
	final int WIDTH = 1024;
	final int HEIGHT = 400;
	final int ROW = 0;
	final int COL = 1;
	
	int row;
	int col;
	int blank_idx;
	
    // JNI functions
	public native int open();
	public native int ioctl(int fd, int type);
	public native int close(int fd);
	int fd;	
  
	class BackThread extends Thread{
		public void run(){
			while(true){
				// Get data from kernel with JNI -> if return 1, then means end game -> finish this activity
                if(ioctl(fd, 2) == 1){
					EndGame();
				}
				try{
					Thread.sleep(1000);
				}
				catch(InterruptedException e){
					
				}
			}
		}
	}
	
    // Load .so file
	static { System.loadLibrary("dev_driver"); };
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main2);
		linear = (LinearLayout)findViewById(R.id.container);
		
		data=(EditText)findViewById(R.id.editText1);
		Button btn=(Button)findViewById(R.id.button1);
		ltn1=new OnClickListener(){
			public void onClick(View v){
				// Parse data from edit text, so we can find row, col
                String temp = data.getText().toString();
				
				row = Integer.parseInt(temp.split(" ")[0]);
				col = Integer.parseInt(temp.split(" ")[1]);
			
                // Make puzzle
				MakePuzzle();
			}
		};
		btn.setOnClickListener(ltn1);
		
        // Thread start
		mThread = new BackThread();
		mThread.setDaemon(true);
		mThread.start();
		
        // Open kernel module with JNI
		fd = open();
	}
	
	protected void MakePuzzle(){
		blank_idx = row * col;
		
		for(int i = 0; i < row; i++){
			LinearLayout rows = new LinearLayout(this);
			rows.setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));
			
			for(int j = 0; j < col; j++){
				int number = (j+1)+(i*col);
				
                // make button dynamically
				Button btnTag = new Button(this);
				btnTag.setLayoutParams(new LayoutParams(WIDTH/col, HEIGHT/row));
				btnTag.setId(number);
				btnTag.setBackgroundColor(Color.LTGRAY);
				
                // set data with row, col to Button
				int[] info = new int[2];
				info[ROW] = i;
				info[COL] = j;
				btnTag.setTag(info);
				btnTag.setText("" + number);
				
				if(number == blank_idx) {
					// Set blank
                    btnTag.setBackgroundColor(Color.BLACK);
				}

				ltn2 = new OnClickListener(){
					public void onClick(View v){
                        // check button can swap and whether game finished
						if(SwapPuzzle(v.getId(), true) && EndPuzzle()) 
							EndGame();
					}
				};
				btnTag.setOnClickListener(ltn2);
				rows.addView(btnTag);
			}
			linear.addView(rows);
		}
		
        // Shuffle puzzle randomly
		for(int i = 0; i < 30; i++){
			int rand = (int)(Math.random() * (row * col) + 1);
			SwapPuzzle(rand, false);
			// if puuzle unshuffled, the shuffle until puzzle is shffuled
            if(EndPuzzle()) i = 0;
		}
		
	}
	
	protected boolean SwapPuzzle(int id, boolean iscount){
		Button blank = (Button)findViewById(blank_idx);
		Button push = (Button)findViewById(id);
		
		int[] blank_info = (int [])blank.getTag();
		int[] push_info = (int [])push.getTag();
		
		boolean canChange = false;
		canChange |= (blank_info[ROW] == push_info[ROW]-1 && blank_info[COL] == push_info[COL]); // BLANK UP
		canChange |= (blank_info[ROW] == push_info[ROW]+1 && blank_info[COL] == push_info[COL]); // BLANK DOWN
		canChange |= (blank_info[ROW] == push_info[ROW] && blank_info[COL] == push_info[COL]-1); // BLANK LEFT
		canChange |= (blank_info[ROW] == push_info[ROW] && blank_info[COL] == push_info[COL]+1); // BLANK RIGHT
	
		if(canChange){
            // Change blank and button information
			blank_idx = push.getId();
			String text = blank.getText().toString();			
			
			blank.setText(push.getText());
			blank.setBackgroundColor(Color.LTGRAY);
			push.setText(text);
			push.setBackgroundColor(Color.BLACK);
			
			if(iscount){
                // send data to kernel with JNI -> (in kernel : check mode and then up counter)
				ioctl(fd, 1);
			}
		}
		return canChange;
	}
	
	protected boolean EndPuzzle(){
        // Check the puzzle is cleared
		boolean isend = true;
		if (blank_idx != row * col) 
			isend = false;
		
		for(int i = 1; i <= row * col; i++){
			Button btn = (Button)findViewById(i);
			int[] info = (int [])btn.getTag();
			if(Integer.parseInt(btn.getText().toString()) != (info[COL]+1)+(info[ROW]*col)){
				isend = false;
				break;
			}
		}
		return isend;
	}
	
	protected void EndGame(){
        // End this Activity
		finish();
        // Open kernel module with JNI
		close(fd);
	}
}
