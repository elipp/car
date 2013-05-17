
#include "common.h"
#include "shader.h"

static char logbuffer[1024];

#define RED_BOLD "\033[1;31m"
#define COLOR_RESET "\033[0m"

ShaderProgram::ShaderProgram(const std::string &name_base) { 	

	for (int i = 0; i < 5; i++) shaderObjIDs[i] = SHADER_NONE;	

	bad = false;

	id_string = name_base;
	shader_filenames[VertexShader] = name_base + "/vs";
	shader_filenames[TessellationControlShader] = name_base + "/tcs";
	shader_filenames[TessellationEvaluationShader] = name_base + "/tes";
	shader_filenames[GeometryShader] = name_base + "/gs";
	shader_filenames[FragmentShader] = name_base + "/fs";

	GLsizei vs_len, tcs_len, tes_len, gs_len, fs_len;

	char *vs_buf = NULL, 
	     *tcs_buf = NULL,
	     *tes_buf = NULL,
	     *gs_buf = NULL, 
	     *fs_buf = NULL;

	
	// note: program will leak memory on error, but then again these kinds of errors are considered "fatal"

	// read applicable shader source files into buffers, run glCreateShader

	vs_buf = ShaderProgram::readShaderFromFile(shader_filenames[VertexShader], &vs_len);
	if (!vs_buf) { set_bad(); goto cleanup; }	// vertex shader is mandatory
	shaderObjIDs[VertexShader] = glCreateShader(GL_VERTEX_SHADER);

	tcs_buf = ShaderProgram::readShaderFromFile(shader_filenames[TessellationControlShader], &tcs_len);
	if (tcs_buf) {
		shaderObjIDs[TessellationControlShader] = glCreateShader(GL_TESS_CONTROL_SHADER);

		tes_buf = ShaderProgram::readShaderFromFile(shader_filenames[TessellationEvaluationShader], &tes_len);
		if (tes_buf) { 
			shaderObjIDs[TessellationEvaluationShader] = glCreateShader(GL_TESS_EVALUATION_SHADER);
		}	
		else {
			logWindowOutput("ShaderProgram error: %s: TessellationControlShader enabled but no TessellationEvaluationShader provided.\n", name_base.c_str());
			set_bad(); goto cleanup;
		}
	}

	gs_buf = ShaderProgram::readShaderFromFile(shader_filenames[GeometryShader], &gs_len);
	if (gs_buf) {
		shaderObjIDs[GeometryShader] = glCreateShader(GL_GEOMETRY_SHADER);
	}

	fs_buf = ShaderProgram::readShaderFromFile(shader_filenames[FragmentShader], &fs_len);
	if (!fs_buf) { set_bad(); goto cleanup; } // fragment shader is mandatory
	shaderObjIDs[FragmentShader] = glCreateShader(GL_FRAGMENT_SHADER);

	// shader sources
	glShaderSource(shaderObjIDs[VertexShader], 1, (const GLchar**)&vs_buf,  (const GLint*)&vs_len);

	if (shaderObjIDs[TessellationControlShader] != SHADER_NONE) {
		glShaderSource(shaderObjIDs[TessellationControlShader], 1, (const GLchar**)&tcs_buf, (const GLint*)&tcs_len);
	}
	if (shaderObjIDs[TessellationEvaluationShader] != SHADER_NONE) {
		glShaderSource(shaderObjIDs[TessellationEvaluationShader], 1, (const GLchar**)&tes_buf, (const GLint*)&tes_len);
	}
	
	if (shaderObjIDs[GeometryShader] != SHADER_NONE) {
		glShaderSource(shaderObjIDs[GeometryShader], 1, (const GLchar**)&gs_buf, (const GLint*)&gs_len);
	}
	glShaderSource(shaderObjIDs[FragmentShader], 1, (const GLchar**)&fs_buf, (const GLint*)&fs_len);

	// compile everything
	for (int i = VertexShader; i <= FragmentShader; i++) {
		if (shaderObjIDs[i] != SHADER_NONE) { 
			glCompileShader(shaderObjIDs[i]);
		}
	}
	programHandle = glCreateProgram();              

	// attach
	
	for (int i = VertexShader; i <= FragmentShader; i++) {
		if (shaderObjIDs[i] != SHADER_NONE) {
			glAttachShader(programHandle, shaderObjIDs[i]);
		}
	}

	glBindAttribLocation(programHandle, 0, "Position_VS_in");
	glBindAttribLocation(programHandle, 1, "Normal_VS_in");
	glBindAttribLocation(programHandle, 2, "TexCoord_VS_in");

	glLinkProgram(programHandle);
	glUseProgram(programHandle);

	if (!checkShaderCompileStatus_all()) 
	{
		set_bad();
		goto cleanup;
	}
	if (!checkProgramLinkStatus()) {
		set_bad();
		goto cleanup;
	}

	#define my_delete_arr(arr) if(arr) delete [] arr; arr = NULL;
cleanup:
	my_delete_arr(vs_buf);
	my_delete_arr(tcs_buf);
	my_delete_arr(tes_buf);
	my_delete_arr(gs_buf); 
	my_delete_arr(fs_buf);

	printStatus();
}

inline const char* shader_present(const GLuint *objIDs, GLint index) {
	if (objIDs[index] != SHADER_NONE) { return "yes"; }
	else { return "no"; }
}

void ShaderProgram::printStatus () const {
	logWindowOutput("ShaderProgram %s status:\n", id_string.c_str());
	logWindowOutput("shader\t\t\tpresent?\tid\n");
	logWindowOutput("Vertex shader: \t\t%s\t\t%d\n", shader_present(shaderObjIDs, VertexShader), shaderObjIDs[VertexShader]);
	logWindowOutput("TessCtrl shader: \t%s\t\t%d\n", shader_present(shaderObjIDs, TessellationControlShader), shaderObjIDs[TessellationControlShader]);
	logWindowOutput("TessEval shader: \t%s\t\t%d\n", shader_present(shaderObjIDs, TessellationEvaluationShader), shaderObjIDs[TessellationEvaluationShader]);
	logWindowOutput("Geometry shader: \t%s\t\t%d\n", shader_present(shaderObjIDs, GeometryShader), shaderObjIDs[GeometryShader]);
	logWindowOutput("Fragment shader: \t%s\t\t%d\n", shader_present(shaderObjIDs, FragmentShader), shaderObjIDs[FragmentShader]);
	logWindowOutput("bad flag: %d\n\n", bad);
}

char* ShaderProgram::readShaderFromFile(const std::string &filename, GLsizei *filesize)
{
	std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);

	if (!in.is_open()) { 
		logWindowOutput( "(warning: ShaderProgram: couldn't open file %s: no such file or directory)\n", filename.c_str());
		return NULL; 
	}
	size_t length = cpp_getfilesize(in);
	*filesize = length;
	char *buf = new char[length+1];
	in.read(buf, length);
	in.close();

	buf[length] = '\0';

	return buf;
}

GLint ShaderProgram::checkShaderCompileStatus_all() // GL_COMPILE_STATUS
{	
	// should check GL_LINK_STATUS as well (glGetProgramInfoLog for linker)
	GLint succeeded[5] = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };
	GLchar *log_buffers[5] = { NULL, NULL, NULL, NULL, NULL };
	GLint num_errors = 0;

	for (int i = VertexShader; i <= FragmentShader; i++) {
		if (shaderObjIDs[i] != SHADER_NONE) {
			glGetShaderiv(shaderObjIDs[i], GL_COMPILE_STATUS, &succeeded[i]);

			if (succeeded[i] == GL_FALSE)
			{	
				logWindowOutput( "glGetShaderiv returned GL_FALSE for query GL_COMPILE_STATUS for shader %s, id: %d\n", shader_filenames[i].c_str(), shaderObjIDs[i]);
				GLint log_length = 0;
				++num_errors;
				glGetShaderiv(shaderObjIDs[i], GL_INFO_LOG_LENGTH, &log_length);
				log_buffers[i] = new GLchar[log_length + 1];
				memset(log_buffers[i], 0, log_length);

				glGetShaderInfoLog(shaderObjIDs[i], log_length, NULL, log_buffers[i]);

				log_buffers[i][log_length-1] = '\0';

				std::ofstream logfile("shader.log", std::ios::out | std::ios::app);	
				logfile << log_buffers[i];
				logfile.close();

				// also print to Visual Studio output window
			}
		}
	}

	if (num_errors > 0) {	

		logWindowOutput( "\nShader %s: error log (glGetShaderInfoLog):\n-----------------------------------------------------------------\n\n", id_string.c_str());
		for (int i = VertexShader; i <= FragmentShader; i++) {
			if (succeeded[i] != GL_TRUE) {
				logWindowOutput( "filename: %s\n\n", shader_filenames[i].c_str());
				logWindowOutput( "%s\n\n", log_buffers[i]);
			}
		}
		logWindowOutput( "\n---------------------------------------------------\n");
	}
	for (int i = VertexShader; i <= FragmentShader; i++) {	
		if (log_buffers[i]) delete [] log_buffers[i];
		log_buffers[i] = NULL;
	}
	if (num_errors > 0) { return false; }
	else { return true; }
}

GLint ShaderProgram::checkProgramLinkStatus() {
	GLint status;
	GLsizei log_len;
	glGetProgramiv(programHandle, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		glGetProgramInfoLog(programHandle, sizeof(logbuffer), &log_len, logbuffer);
		logWindowOutput( "ShaderProgram::checkProgramLinkStatus: shader program link error. Log:\n%s\n", logbuffer);
		return 0;
	}
	else {
		glGetProgramInfoLog(programHandle, sizeof(logbuffer), &log_len, logbuffer);
		logWindowOutput( "Program %s; LINK:\n", id_string.c_str());
		if (log_len > 0) { logWindowOutput("log: \n"); 
						   logWindowOutput("%s\n\n", logbuffer);
		}
		else { logWindowOutput( "<OK>\n\n"); }

		glValidateProgram(programHandle);
		glGetProgramInfoLog(programHandle, sizeof(logbuffer), &log_len, logbuffer);
		logWindowOutput( "Program %s; VALIDATION:\n\n", id_string.c_str());
		if (log_len > 0) {  logWindowOutput("log: \n"); 
							logWindowOutput("%s\n\n", logbuffer);
		}
		else { logWindowOutput("<OK>\n\n"); }

		logWindowOutput("\n");
		return 1;
	}
}

void ShaderProgram::construct_uniform_map() {
	GLint total = -1;
	glGetProgramiv(programHandle, GL_ACTIVE_UNIFORMS, &total);
#define UNIFORM_NAME_LEN_MAX 64
	char uniform_name_buf[UNIFORM_NAME_LEN_MAX];
	memset(uniform_name_buf, 0, UNIFORM_NAME_LEN_MAX);

	for (GLuint i = 0; i < total; i++) {
		GLsizei name_len;
		GLint num;
		GLenum type = GL_ZERO;
		glGetActiveUniform(programHandle, i, sizeof(uniform_name_buf) - 1, &name_len, &num, &type, uniform_name_buf);
		uniform_name_buf[name_len] = '\0';
		GLuint loc = glGetUniformLocation(programHandle, uniform_name_buf);
		uniforms.insert(std::pair<std::string, int>(std::string(uniform_name_buf), loc));
		logWindowOutput( "construct_uniform_map: %s: inserted uniform %s with location %d\n", id_string.c_str(), uniform_name_buf, loc);
	}
}


void ShaderProgram::update_uniform_mat4(const std::string &uniform_name, const void* data) {
	glUseProgram(programHandle);
	std::unordered_map<std::string,GLuint>::iterator iter = uniforms.find(uniform_name);
	if (iter == uniforms.end()) {
		//logWindowOutput( "update_uniform: warning: uniform \"mat4 %s\" not active in shader %s\n!", uniform_name.c_str(), id_string.c_str());
		return;
	}
	else {
		GLuint uniform_location = iter->second;
		glUniformMatrix4fv(uniform_location, 1, GL_FALSE, (const GLfloat*)data);
	}
}
void ShaderProgram::update_uniform_vec4(const std::string &uniform_name, const void *data) {
	glUseProgram(programHandle);
	std::unordered_map<std::string,GLuint>::iterator iter = uniforms.find(uniform_name);
	if (iter == uniforms.end()) {
		//logWindowOutput( "update_uniform: warning: uniform \"vec4 %s\" not active in shader %s\n!", uniform_name.c_str(), id_string.c_str());
		return;
	}
	else {
		GLuint uniform_location = iter->second;
		glUniform4fv(uniform_location, 1, (const GLfloat*)data);
	}
}
void ShaderProgram::update_uniform_1f(const std::string &uniform_name, GLfloat value){
	glUseProgram(programHandle);
	std::unordered_map<std::string,GLuint>::iterator iter = uniforms.find(uniform_name);
	if (iter == uniforms.end()) {
		//logWindowOutput( "update_uniform: warning: uniform \"float %s\" not active in shader %s\n!", uniform_name.c_str(), id_string.c_str());
		return;
	}
	else {
		GLuint uniform_location = iter->second;
		glUniform1f(uniform_location, value);
	}
}

void ShaderProgram::update_uniform_1i(const std::string &uniform_name, GLint value){
	glUseProgram(programHandle);
	std::unordered_map<std::string,GLuint>::iterator iter = uniforms.find(uniform_name);
	if (iter == uniforms.end()) {
		//logWindowOutput( "update_uniform: warning: uniform \"int %s\" not active in shader %s\n!", uniform_name.c_str(), id_string.c_str());
		return;
	}
	else {
		GLuint uniform_location = iter->second;
		glUniform1i(uniform_location, value);
	}
}


/*	std::ofstream logfile("shader.log", std::ios::out | std::ios::app);

	logfile << "compilation of GLSL shader source file "<< filename << " failed. Contents: \n\n";

	logfile.write(buf, length);
	logfile.put('\n');
	logfile.put('\n');
	logfile.close();
 */
