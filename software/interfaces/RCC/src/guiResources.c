/**
 * guiResources.c
 *
 * Text, primitives, and color resources for the GUI to draw
 */
#include "rcc.h"

const GLfloat color_red[COLOR_SIZE] = { 0.9, 0.0, 0.0 };
const GLfloat color_lightred[COLOR_SIZE] = { 0.9, 0.5, 0.5 };

const GLfloat color_blue[COLOR_SIZE] = { 0.2, 0.4, 1.0 };
const GLfloat color_lightblue[COLOR_SIZE] = { 0.2, 0.7, 1 };

const GLfloat color_purple[COLOR_SIZE] = { 0.8, 0.2, 1.0 };
const GLfloat color_lightpurple[COLOR_SIZE] = { 0.8, 0.7, 1.0 };

const GLfloat color_pink[COLOR_SIZE] = { 1.0, 0.2, 0.8 };
const GLfloat color_lightpink[COLOR_SIZE] = { 1.0, 0.7, 0.8 };

const GLfloat color_orange[COLOR_SIZE] = { 1.0, 0.4, 0.2 };
const GLfloat color_lightorange[COLOR_SIZE] = { 1.0, 0.7, 0.5 };

const GLfloat *color_array[NUMROBOT_POINTS] = {color_red, color_blue, color_purple, color_pink, color_orange};
const GLfloat *lightcolor_array[NUMROBOT_POINTS] = {color_lightred, color_lightblue, color_lightpurple, color_lightpink, color_lightorange};

const GLfloat color_black[COLOR_SIZE] = { 0.1, 0.1, 0.1 };
const GLfloat color_grey[COLOR_SIZE] = { 0.8, 0.8, 0.8 };
const GLfloat color_darkgrey[COLOR_SIZE] = { 0.5, 0.5, 0.5 };
const GLfloat color_white[COLOR_SIZE] = { 1.0, 1.0, 1.0 };

GLint drawingListBase;

GLuint base, baseOut;
GLYPHMETRICSFLOAT gmf[256];
GLYPHMETRICSFLOAT gmfOut[256];

static int alignment = ALIGN_LEFT;
static GLfloat textWidth = TEXT_MED, textHeight = TEXT_MED;

/**
 * Initialize drawing object list
 */
void drawInit()
{
	glPointSize(POINT_SIZE);
	glLineWidth(LINE_WIDTH);
	glClearColor(1.0, 1.0, 1.0, 1.0);

	/* Enable antialias */
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

	/* Create quadratic object */
	GLUquadricObj *qobj;
	qobj = gluNewQuadric();

	drawingListBase = glGenLists(NUM_DRAWING_LISTS);

	glNewList(LIST_CIRCLE_FILLED, GL_COMPILE);
		gluQuadricDrawStyle(qobj, GLU_FILL);
		gluDisk(qobj, 0.0, 1.0, DISK_SLICES, DISK_LOOPS);
		gluDisk(qobj, 0.0, 1.0, DISK_SLICES, DISK_LOOPS);
		gluQuadricDrawStyle(qobj, GLU_SILHOUETTE);
		gluDisk(qobj, 0.0, 1.0, DISK_SLICES, DISK_LOOPS);
		gluDisk(qobj, 0.0, 1.0, DISK_SLICES, DISK_LOOPS);
	glEndList();

	glNewList(LIST_SQUARE, GL_COMPILE);
		gluQuadricDrawStyle(qobj, GLU_FILL);
		glRectf(-1, -1, 1, 1);
		gluQuadricDrawStyle(qobj, GLU_SILHOUETTE);
		glRectf(-1, -1, 1, 1);
		glBegin(GL_LINE_LOOP);
			glVertex2f(-1, -1);
			glVertex2f(1, -1);
			glVertex2f(1, 1);
			glVertex2f(-1, 1);
		glEnd();
	glEndList();

	gluDeleteQuadric(qobj);
}

/**
 * Initialize the text for rendering on the GUI
 */
GLvoid textInit(GLvoid)
{
	HDC hdc = wglGetCurrentDC();
	HFONT font;

	/* Create our font */
	font = CreateFont(-12,
					  0,
					  0,
					  0,
					  FW_ULTRALIGHT,
					  FALSE,
					  FALSE,
					  FALSE,
					  ANSI_CHARSET,
					  OUT_TT_PRECIS,
					  CLIP_DEFAULT_PRECIS,
					  ANTIALIASED_QUALITY,
					  FF_DONTCARE | DEFAULT_PITCH,
					  "Lucida Console");

	SelectObject(hdc, font);

	base = glGenLists(256);
	baseOut = glGenLists(256);

	/* Create the font and font outline (for antialias) */
	wglUseFontOutlines(hdc,
					   0,
					   255,
					   base,
					   0.0,
					   0.0,
					   WGL_FONT_POLYGONS,
					   gmf);
	wglUseFontOutlines(hdc,
					   0,
					   255,
					   baseOut,
					   0.0,
					   0.0,
					   WGL_FONT_LINES,
					   gmfOut);

	DeleteObject(font);
}

/**
 * Output text onto the GUI
 */
GLfloat textPrintf(const char *fmt, ...)
{
	unsigned int i;
	GLfloat length = 0;
	char text[256] = { 0 };
	va_list ap;

	if (fmt == NULL)
		return (-1);

	va_start(ap, fmt);
		vsprintf(text, fmt, ap);
	va_end(ap);

	for (i = 0; i < strlen(text); i++)
		length += gmf[(int) text[i]].gmfCellIncX;

	glPushMatrix();
		glScalef(textWidth, textHeight, 0);

		/* Translate for alignment */
		switch (alignment)
		{
		case ALIGN_RIGHT: {
			glTranslatef(-length, 0.0, 0.0);
			break;
		}
		case ALIGN_CENTER: {
			glTranslatef(-length / 2, 0.0, 0.0);
			break;
		}
		case ALIGN_LEFT:
		default: {
			break;
		}
		}

		glPushAttrib(GL_LIST_BIT);
		glPushMatrix();
			glListBase(base);
			glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
		glPopMatrix();

		if (textWidth != TEXT_LARGE)
			glLineWidth(LINE_WIDTH_SMALL);

		glPushMatrix();
			glListBase(baseOut);
			glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
		glPopMatrix();

		glLineWidth(LINE_WIDTH);

		glPopAttrib();
	glPopMatrix();

	return (length);
}

/**
 * Set current alignment of the text
 */
void textSetAlignment(int ta)
{
	alignment = ta;
}

/**
 * Set the current size of the text
 */
void textSetSize(GLfloat size)
{
	textWidth = size;
	textHeight = size;
}
