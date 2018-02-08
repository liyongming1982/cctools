/*
 Copyright (C) 2018- The University of Notre Dame
 This software is distributed under the GNU General Public License.
 See the file COPYING for details.
 */

#include "dag_file.h"
#include "makeflow_log.h"
#include "makeflow_hook.h"

#include "batch_task.h"
#include "batch_wrapper.h"

#include "debug.h"
#include "list.h"
#include "stringtools.h"
#include "xxmalloc.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

static int makeflow_module_sandbox_node_submit(struct dag_node *node, struct batch_task *task){
	struct batch_wrapper *wrapper = batch_wrapper_create();
	char *cmd = string_format("task_%d", task->taskid);
	batch_wrapper_prefix(wrapper, cmd);
	free(cmd);

	/* Save the directory we were originally working in. */
	//batch_wrapper_pre(wrapper, "export CUR_WORK_DIR=$(pwd)");

	/* Create sandbox. This should probably have a hex or random tail to be unique. */
	cmd = string_format("mkdir task_%d_sandbox", task->taskid);
	batch_wrapper_pre(wrapper, cmd);
	free(cmd);

	struct batch_file *f;
	list_first_item(task->input_files);
	while((f = list_next_item(task->input_files))){
		/* Add a cp for each file. Not linking as wq may already have done this. Not moving as it may be local. */
		cmd = string_format("cp %s task_%d_sandbox/%s", f->inner_name, task->taskid, f->inner_name);
		batch_wrapper_pre(wrapper, cmd);
		free(cmd);
	}
	/* Enter into sandbox_dir. */
	cmd = string_format("cd task_%d_sandbox", task->taskid);
	batch_wrapper_pre(wrapper, cmd);
	free(cmd);

	/* Execute the previous levels commmand. */
	cmd = string_format("sh -c \"%s\"", task->command);
	batch_wrapper_cmd(wrapper, cmd);
	free(cmd);

	/* Once the command is finished go back to working dir. */
	batch_wrapper_post(wrapper, "cd ..");
	//batch_wrapper_post(wrapper, "cd $CUR_WORK_DIR");

	list_first_item(task->output_files);
	while((f = list_next_item(task->output_files))){
		/* Copy out results to expected location. */
		cmd = string_format("cp task_%d_sandbox/%s %s", task->taskid, f->inner_name, f->inner_name);
		batch_wrapper_post(wrapper, cmd);
		free(cmd);
	}

	/* Remove and fully wipe out sandbox. */
	cmd = string_format("rm -rf task_%d_sandbox", task->taskid);
	batch_wrapper_post(wrapper, cmd);
	free(cmd);

	cmd = batch_wrapper_write(wrapper, task);
	if(cmd){
		batch_task_set_command(task, cmd);
		struct dag_file *df = makeflow_hook_add_input_file(node->d, task, cmd, cmd);
		debug(D_MAKEFLOW_HOOK, "Wrapper written to %s", df->filename);
		makeflow_log_file_state_change(node->d, df, DAG_FILE_STATE_EXISTS);
	} else {
		debug(D_MAKEFLOW_HOOK, "Failed to create wrapper %d, %s", errno, strerror(errno));
	}

	return MAKEFLOW_HOOK_SUCCESS;
}

struct makeflow_hook makeflow_hook_sandbox = {
	.module_name = "Sandbox",
	.node_submit = makeflow_module_sandbox_node_submit,
};


