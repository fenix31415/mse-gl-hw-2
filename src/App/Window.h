#pragma once

#include <Base/GLWidget.hpp>

#include <QElapsedTimer>
#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>

#include <functional>
#include <memory>

struct Primitive {
	std::unique_ptr<QOpenGLTexture> tex;
	std::unique_ptr<QOpenGLTexture> normals;
	int indices_offset;
	int indices_size;
};

struct Vertex {
	QVector3D pos;
	QVector3D normal;
	QVector2D tex;
	QVector3D tangent;
	QVector3D bitangent;
};

namespace tinygltf
{
class Model;
}

class Window final : public fgl::GLWidget
{
	Q_OBJECT
public:
	Window() noexcept;
	~Window() override;

public:// fgl::GLWidget
	void onInit() override;
	void onRender() override;
	void onResize(size_t width, size_t height) override;
	void mouseMoveEvent(QMouseEvent * e) override;
	void mousePressEvent(QMouseEvent * e) override;
	void mouseReleaseEvent(QMouseEvent *) override;
	void keyPressEvent(QKeyEvent * e) override;
	void keyReleaseEvent(QKeyEvent * event) override;
	void updateMoving();

private:
	void process_node(const tinygltf::Model & model, int32_t node_ind, std::vector<Vertex> & model_vertices, std::vector<GLuint> & model_indices, QMatrix4x4 parent_transform = QMatrix4x4(), int parent_texture = -1);

	class PerfomanceMetricsGuard final
	{
	public:
		explicit PerfomanceMetricsGuard(std::function<void()> callback);
		~PerfomanceMetricsGuard();

		PerfomanceMetricsGuard(const PerfomanceMetricsGuard &) = delete;
		PerfomanceMetricsGuard(PerfomanceMetricsGuard &&) = delete;

		PerfomanceMetricsGuard & operator=(const PerfomanceMetricsGuard &) = delete;
		PerfomanceMetricsGuard & operator=(PerfomanceMetricsGuard &&) = delete;

	private:
		std::function<void()> callback_;
	};

private:
	[[nodiscard]] PerfomanceMetricsGuard captureMetrics();

signals:
	void updateUI();

private:
	GLint mvpUniform_ = -1;
	GLint modelUniform_ = -1;
	GLint viewUniform_ = -1;
	GLint ambientStrengthUniform_ = -1;
	GLint diffuseReflectionUniform_ = -1;
	GLint Light1ParamUniform_ = -1;
	GLint Light2ParamUniform_ = -1;
	GLint shininessUniform_ = -1;
	GLint specularUniform_ = -1;
	GLint cameraPosUniform_ = -1;
	GLint timeValueUniform_ = -1;
	GLint morphSpeedUniform_ = -1;

	QOpenGLBuffer vbo_{QOpenGLBuffer::Type::VertexBuffer};
	QOpenGLBuffer ibo_{QOpenGLBuffer::Type::IndexBuffer};
	QOpenGLVertexArrayObject vao_;

	QMatrix4x4 model_;
	QMatrix4x4 view_;
	QMatrix4x4 projection_;

	bool dragging_ = false;
	QPoint lastMousePos_;
	float cameraRotationX = 21.5f;
	float cameraRotationY = 13.1f;
	QVector3D cameraPosition = {0.16f, -0.31f, -0.81f};

	float ambientStrength_ = 0.5f;
	float diffuseReflection_ = 1;
	float Light1Param_ = 0.9f;
	float Light2Param_ = 1;
	float shininess_ = 30;
	float specular_ = 1;
	float morphSpeed_ = 0.2f;

	std::unique_ptr<QOpenGLShaderProgram> program_;
	std::vector<Primitive> primitives_data;

	// W A S D Ctrl Space
	bool buttons_[6] = {};

	QElapsedTimer timer_;
	size_t frameCount_ = 0;

	struct {
		size_t fps = 0;
	} ui_;

	bool animated_ = true;
};
